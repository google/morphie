// Copyright 2015 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations under
// the License.

// A labelled graph is one with labels attached to nodes, edges and to the
// graph. Nodes and edges have unique identifiers which are distinct from their
// labels. Node and edge labels are not necessarily unique.
//
// Example. The graph below has three nodes with ids 1, 2 and 3 and two edges
// with ids 'a' and 'b'. Nodes 1 and 2 have the same label because they
// represent multiple instances of a 'System' event at a certain time. Node 3
// represents a filename and is unique. The edge labels describe the
// relationship between the event and the file.
//
// [1:event(1370044800, System)] --a:create--> [3:file(/tmp/foo/bar.txt)]
//                                              ^
// [2:event(1370044800, System)] --b:modify-----|
//
// The LabeledGraph class provides an API for constructing and manipulating
// graphs with typed labels. The type of a label and the values of these labels
// are both represented as Abstract Syntax Tree (AST) protos defined in
// third_party/logle/ast.proto.
//
// Node labels can be declared unique, which means that exactly one node in the
// graph can have that label. For example, in graphs constructed for forensic
// analysis, nodes representing filenames, IP addresses and URLs are unique,
// while nodes representing process execution and filesystem events are not.
//
// Edge labels can be declared unique, which means that for a given pair of
// nodes, there can be exactly one edge with that label. For example, in a graph
// that summarizes network connections between machines, edges labelled "SSH"
// or "FTP" will be unique, but in a more detailed graph there could be one edge
// for each connection.
//
// The design doc for this package is at go/logle-graph. This code is not thread
// safe.
#ifndef LOGLE_LABELED_GRAPH_H_
#define LOGLE_LABELED_GRAPH_H_

#include <stddef.h>

#include <boost/functional/hash/hash.hpp>
#include <boost/graph/directed_graph.hpp>
#include <set>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "ast.pb.h"
#include "base/string.h"
#include "type_checker.h"
#include "util/status.h"


namespace logle {

using std::set;
using std::unordered_map;

// The declaration below defines a Graph type using the Boost Graph Library. A
// graph is represented as an adjacency_list. The set of nodes and set of edges
// adjacent to a node are represented as std::vectors (boost::vecS). Node labels
// and edge labels are TaggedASTs and the graph label is an AST.
using Graph = ::boost::adjacency_list<::boost::vecS, ::boost::vecS,
                                      ::boost::bidirectionalS, TaggedAST,
                                      TaggedAST, AST>;
using NodeId = ::boost::graph_traits<Graph>::vertex_descriptor;
using EdgeId = ::boost::graph_traits<Graph>::edge_descriptor;
// An Edge consists of a source node, a target node and a string serialization
// of the AST representing the edge label.
struct Edge {
  Edge(NodeId src, NodeId tgt, const string& lbl)
      : source(src), target(tgt), label(lbl) {}

  friend bool operator==(const Edge& a, const Edge& b) {
    return std::tie(a.source, a.target, a.label) ==
           std::tie(b.source, b.target, b.label);
  }

  NodeId source;
  NodeId target;
  string label;
};
// The hash function used by indexes that have edges as keys.
struct EdgeHash {
 public:
  size_t operator()(const Edge& edge) const {
    std::size_t seed = 0;
    boost::hash_combine(seed, edge.source);
    boost::hash_combine(seed, edge.target);
    boost::hash_combine(seed, edge.label);
    return seed;
  }
};
// See the comments of the NodeSetBegin() method for more on these iterators.
using NodeIterator = ::boost::graph_traits<Graph>::vertex_iterator;
using EdgeIterator = ::boost::graph_traits<Graph>::edge_iterator;
// The Boost Graph Library has separate iterators for incoming and outgoing
// edges. The type aliases below are for consistency with the rest of this code.
// A 'Range' is a pair of iterators representing the beginning and end of a
// collection.
using InEdgeIterator = ::boost::graph_traits<Graph>::in_edge_iterator;
using InEdgeRange = std::pair<InEdgeIterator, InEdgeIterator>;
using OutEdgeIterator = ::boost::graph_traits<Graph>::out_edge_iterator;
using OutEdgeRange = std::pair<OutEdgeIterator, OutEdgeIterator>;
// A Graph object internally contains a map from nodes and edges to labels. An
// index is a map from labels to sets of nodes or sets of edges. For nodes with
// unique labels, the index maps labels to nodes. The key in an index is a
// serialization of an AST proto.
template <typename ObjectT>
using Index = unordered_map<string, ObjectT>;
// There is one index for each type of node or edge label. A key in the Indexes
// map is a string like "File" representing a tag in a TaggedAST. Importantly, a
// key in Indexes, is not a serialization of a proto.
template <typename ObjectT>
using Indexes = unordered_map<string, Index<ObjectT>>;
// The EdgeIndex below is used for unique edge labels. It is defined separately
// from the index types above because it uses a custom hash function.
using EdgeIndex = unordered_map<Edge, EdgeId, EdgeHash>;
using UniqueEdges = unordered_map<string, EdgeIndex>;

// A LabeledGraph object stores the following data: nodes, edges, a set of node
// label types, a set of edge label types, a graph label type, a marking of node
// and edge label types as unique, a map from nodes and edges to their labels
// and a set of indexes. An entry in the index maps a label to the set nodes and
// set of edges with that label. Functions in LabeledGraph allow querying and
// manipulating this data.
//
// A LabeledGraph object can be in two states: uninitialized and initialized.
// The graph is initialized by calling LabeledGraph::Initialize(...) to set the
// node, edge, and graph types and unique node and edge types. Calling member
// functions before initialization will result in a crash.
//
// Example. The code below constructs the example graph at the top of this file.
//
//   // Declare node, edge and graph label types.
//   vector<AST> asts;
//   asts.emplace_back(ast::type::MakeTimestamp("Time", false));
//   asts.emplace_back(ast::type::MakeString("EventType", false));
//   ast::type::Types node_types;
//   node_types.insert({"Event", ast::type::MakeTuple("Event", false, asts)});
//   AST filename = ast::type::MakeList(
//       "File", false, ast::type::MakeString("part", false));
//   node_types.insert({"File", filename});
//   set<string> unique;
//   unique.insert("File");
//   ast::type::Types edge_types;
//   edge_types.insert({"Modifies", ast::type::MakeString("Action", true)});
//   AST graph_type = ast::type::MakeString("MachineName", false);
//   LabeledGraph graph;
//   EXPECT_TRUE(graph.Initialize(
//       node_types, unique, edge_types, set<string>(), graph_type).ok());
class LabeledGraph {
 public:
  // Create an uninitialized labelled graph.
  LabeledGraph() : is_initialized_(false) {}
  // Disallow copying and assignment.
  LabeledGraph(const LabeledGraph&) = delete;
  LabeledGraph& operator=(const LabeledGraph&) = delete;

  // Returns a util::Status object with the following possible error codes:
  // - INVALID_ARGUMENT - if 'node_types' or 'edge_types' contain malformed
  //   types or if the AST graph_type is not initialized. The specific error is
  //   returned in Initialize(..).error_message().
  // - OK - otherwise.
  // The containers 'node_types', 'edge_types', 'unique_nodes' and
  // 'unique_edges' may be empty. If types are constructed using functions in
  // third_party/logle/type.h, the constraints on types will be satisfied. The
  // definition of types and functions for type checking are in
  // third_party/logle/type_checker.h
  util::Status Initialize(ast::type::Types node_types,
                          const set<string>& unique_nodes,
                          ast::type::Types edge_types,
                          const set<string>& unique_edges, AST graph_type);
  ast::type::Types GetNodeTypes() const;
  // Returns the tags of node types that are unique.
  set<string> GetUniqueNodeTags() const;
  // Returns
  // - (true, type), if a node label of type 'type' tagged with 'tag' was
  //   declared when the graph was initialized.
  // - (false, AST()) otherwise, with the second element uninitialized.
  std::pair<bool, AST> GetNodeType(const string& tag) const;
  ast::type::Types GetEdgeTypes() const;
  // Returns the tags of edge types that are unique.
  set<string> GetUniqueEdgeTags() const;
  // Similar to GetNodeType(..) but for edge types.
  std::pair<bool, AST> GetEdgeType(const string& tag) const;
  // Returns an AST representing the graph type. Unlike node and edge types, a
  // graph type is an AST, not a TaggedAST.
  AST GetGraphType() const;
  // - Crashes if graph_label does not respect the graph label type.
  void SetGraphLabel(AST graph_label);
  // Retrieves the id of a node with the given label. If label.tag() is not
  // declared as unique, a new node is created. Otherwise, a node is created
  // only if a node with this label does not already exist.
  // - Crashes if 'label' is not of a declared node type.
  //
  // Example. Assume the tag 'event' is not unique and 'file' is.
  //   TaggedAST event, file;
  //   // Initialize 'event' and 'file'.
  //   NodeId n1, n2;
  //   n1 = FindOrAddNode(event)
  //   n2 = FindOrAddNode(event)
  //   CHECK(n1 != n2, "The graph should have two nodes labelled 'event'");
  //   n1 = FindOrAddNode(file)
  //   n2 = FindOrAddNode(file)
  //   CHECK(n1 == n2, "The graph should have one node labelled 'file'");
  //
  // A note on complexity: Adding a node with a non-unique label updates an
  // index from labels to sets of nodes. In the worst case, if all nodes have
  // the same label, this operation takes O(h + log(n)) time, where n is the
  // number of graph nodes and h is the complexity of serializing and hashing
  // 'label' to generate an index key.
  NodeId FindOrAddNode(const TaggedAST& label);
  // Retrieve the id of an edge with the given label between the source and
  // target nodes. Behaves like FindOrAddNode for edge creation.
  // - Crashes if 'label' is not of a declared edge type.
  // The note about worst case complexity of FindOrAddNode applies here.
  EdgeId FindOrAddEdge(NodeId source, NodeId target, const TaggedAST& label);
  // Returns true if there is a node with the given identifier in the graph.
  bool HasNode(NodeId node_id) const;
  // Returns true if there is an edge corresponding to a given identifier.  An
  // EdgeId determines a source and a target node and this function returns true
  // if that source and target exist are are connected by an edge.
  bool HasEdge(EdgeId edge_id) const;
  // - Requires that HasNode(node_id) is true of the argument.
  // In the TaggedAST 't' that is returned, t.has_ast() can be false because
  // labels can be null. An empty label is not an error.
  TaggedAST GetNodeLabel(NodeId node_id) const;
  // - Requires that HasEdge(edge_id) is true of the argument.
  // Edge ids obtained by querying this API are guaranteed to be valid.
  TaggedAST GetEdgeLabel(EdgeId edge_id) const;
  // An EdgeId contains a source and target NodeId and these two functions
  // retrieve those values.
  // - The functions require that HasEdge(edge_id) be true.
  NodeId Source(EdgeId edge_id) const;
  NodeId Target(EdgeId edge_id) const;
  // Return the label of the graph. This is an AST, not an TaggedAST.
  AST GetGraphLabel() const;

  bool IsUniqueNodeType(const TaggedAST& label_type) const;
  bool IsUniqueEdgeType(const TaggedAST& label_type) const;
  // Returns the set of nodes with a given label and returns the empty set if no
  // such nodes exist.
  set<NodeId> GetNodes(const TaggedAST& label) const;
  // Returns the set of edges with a given label and returns the empty set if no
  // such nodes exist.
  set<EdgeId> GetEdges(const TaggedAST& label) const;
  // In an edge (u,v), the vertex u is the predecessor and v is the successor.
  // The functions below return the predecessors and successors of a node with a
  // given id. The functions return the empty set if either the node has no
  // predecessors (or successors).
  //  - The functions require that HasNode(node_id) is true.
  set<NodeId> GetPredecessors(NodeId node_id) const;
  set<NodeId> GetSuccessors(NodeId node_id) const;
  // These functions return iterators to the sets of incoming edges to a node or
  // outgoing edges from a node.
  InEdgeIterator InEdgeBegin(NodeId node_id) const;
  InEdgeIterator InEdgeEnd(NodeId node_id) const;
  OutEdgeIterator OutEdgeBegin(NodeId node_id) const;
  OutEdgeIterator OutEdgeEnd(NodeId node_id) const;
  // These functions return the union of the predecessors (or successors) of all
  // nodes with a given label and return the empty set if no such node exists.
  set<NodeId> GetLabelPredecessors(const TaggedAST& label) const;
  set<NodeId> GetLabelSuccessors(const TaggedAST& label) const;

  // Dereferencing these iterators yields a copy of a NodeId or an EdgeId. Boost
  // iterators have type std::iterator_traits<..>::value_type, so though these
  // are not of the STL-type 'const_iterator', the iterators cannot be used to
  // change the contents of the graph.
  NodeIterator NodeSetBegin() const;
  NodeIterator NodeSetEnd() const;
  EdgeIterator EdgeSetBegin() const;
  EdgeIterator EdgeSetEnd() const;
  // Functions that return statistics about nodes and edges.
  int NumNodeTypes() const;
  int NumUniqueNodeTypes() const;
  int NumNodes() const;
  int NumLabeledNodes(const TaggedAST& label) const;
  int NumEdgeTypes() const;
  int NumUniqueEdgeTypes() const;
  int NumEdges() const;
  int NumLabeledEdges(const TaggedAST& label) const;

 private:
  // InsertNode(..) and InsertEdge(...) always modify the graph, unlike the
  // FindOrAdd functions, which might leave the graph unchanged.
  NodeId InsertNode(TaggedAST label);
  EdgeId InsertEdge(NodeId source, NodeId target, TaggedAST label);

  bool is_initialized_;
  ast::type::Types node_types_;
  ast::type::Types edge_types_;
  AST graph_type_;
  AST graph_label_;
  Graph graph_;

  Indexes<set<NodeId>> node_indexes_;
  Indexes<set<EdgeId>> edge_indexes_;
  // A unique label is called a name in this code. For nodes with unique labels,
  // the index maps labels to node ids.
  Indexes<NodeId> named_nodes_;
  UniqueEdges named_edges_;
};

}  // logle

#endif  // LOGLE_LABELED_GRAPH_H_
