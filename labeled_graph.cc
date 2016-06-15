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

// LabeledGraph functions are implemented as a wrapper around Boost Graph
// Library (BGL) functions. This is preferrable to using the BGL directly
// because the BGL heavily uses inheritance and templates which leads to
// programming in a style noticeably different from the rest of this code base.
// Another reason for this wrapper is to make it easy to replace the BGL with a
// different graph library if required at a later point. Moreover, the BGL does
// not provide a convenient way to look up graph elements based on their labels,
// which LabeledGraph does by using indexes.
#include "third_party/logle/labeled_graph.h"

#include <utility>

#include "third_party/logle/ast.h"
#include "third_party/logle/util/logging.h"
#include "third_party/logle/util/string_utils.h"

namespace third_party_logle {

using ast::type::Types;
namespace type = ast::type;

namespace {
const char* const kNullStr = "null";
const char* const kInitializationErr = "The graph is not initialized.";
const char* const kInvalidNodeErr = "Invalid node id.";
const char* const kInvalidEdgeErr = "Invalid edge id.";
const char* const kInvalidIndexTagErr = "There is no index for labels tagged ";

// If a tagged AST has an AST field, return the serialization of the field.
// Otherwise, return the string "null". TaggedAST objects with different tags
// but with no AST field will return the same label string.
//
// Note: If ASTs are serialized in a loop, the MessageLite::SerializeAsString
// documentation recommends using MessageLite::SerializeToString(..) with an
// output string argument to reduce heap fragmentation.
static string GetSerializationOrNull(const TaggedAST& ast) {
  if (ast.has_ast()) {
    return ast.ast().SerializeAsString();
  }
  return kNullStr;
}

// Retrieve the type corresponding to a tag in a Types map.
// - Returns the pair (true, types[tag]), if 'tag' is a key in 'types' and
//   (false, AST()) otherwise.
std::pair<bool, AST> GetTaggedType(const string& tag, const Types& types) {
  const auto type_it = types.find(tag);
  if (type_it == types.end()) {
    return {false, {}};
  }
  return {true, type_it->second};
}

// Add a label and identifier to an index. The identifier may be either a node
// or an edge id and the index must have the corresponding type.
template <typename ObjectId>
util::Status IndexObject(const TaggedAST& label, ObjectId id,
                         Indexes<set<ObjectId>>* indexes) {
  auto index_it = indexes->find(label.tag());
  if (index_it == indexes->end()) {
    return util::Status(Code::INVALID_ARGUMENT,
                        util::StrCat(kInvalidIndexTagErr, label.tag(), "."));
  }
  Index<set<ObjectId>>& index = index_it->second;
  index[GetSerializationOrNull(label)].insert(id);
  return util::Status::OK;
}

// Remove the object 'id' from the index of 'label'. The object may be a node or
// an edge and 'label' must be of non-unique type.
template <typename ObjectId>
void DeIndexObject(const TaggedAST& label, ObjectId id,
                   Indexes<set<ObjectId>>* indexes) {
  auto index_it = indexes->find(label.tag());
  Index<set<ObjectId>>& index = index_it->second;
  index[GetSerializationOrNull(label)].erase(id);
}

// The functions below extend the index of unique nodes or edges with a new
// label, or remove a specific node or edge from a unique index. Unlike the
// situation for non-unique indexes, separate functions are used for
// manipulating unique node and edge indexes. This is because a unique node
// index uses a serialized label as a key while a unique edge index uses a
// triple of a source and target node and a serialized edge label as a key.
util::Status IndexUniqueNode(const TaggedAST& label, NodeId node_id,
                             Indexes<NodeId>* named_nodes) {
  auto index_it = named_nodes->find(label.tag());
  Index<NodeId>& named_node = index_it->second;
  string name = GetSerializationOrNull(label);
  auto name_it = named_node.find(name);
  if (name_it != named_node.end()) {
    return util::Status(
        Code::INVALID_ARGUMENT,
        util::StrCat("A node with label ",
                     ast::ToString(label, ast::PrintOption::kValue),
                     " already exists."));
  }
  named_node.insert({name, node_id});
  return util::Status::OK;
}

void DeIndexUniqueNode(const TaggedAST& label, NodeId node_id,
                       Indexes<NodeId>* named_nodes) {
  auto index_it = named_nodes->find(label.tag());
  Index<NodeId>& named_node = index_it->second;
  string name = GetSerializationOrNull(label);
  auto name_it = named_node.find(name);
  if (name_it == named_node.end()) {
    return;
  }
  named_node.erase(name_it);
}

util::Status IndexUniqueEdge(const string& tag, const Edge& edge,
                             EdgeId edge_id, UniqueEdges* indexes) {
  auto index_it = indexes->find(tag);
  EdgeIndex& index = index_it->second;
  auto name_it = index.find(edge);
  if (name_it != index.end()) {
    return util::Status(Code::INVALID_ARGUMENT, "Unique edge label exists.");
  }
  Edge key(edge.source, edge.target, edge.label);
  index.insert({key, edge_id});
  return util::Status::OK;
}

void DeIndexUniqueEdge(const string& tag, const Edge& edge,
                       UniqueEdges* indexes) {
  auto index_it = indexes->find(tag);
  EdgeIndex& index = index_it->second;
  auto name_it = index.find(edge);
  if (name_it == index.end()) {
    return;
  }
  index.erase(name_it);
}

// Retrieve a set of identifiers from an index given a label. Returns the empty
// set either if no index exists for label.tag(), or if an index exists but does
// not contain the serialization of label.ast() as a key.
template <typename ObjectId>
set<ObjectId> GetLabeledObjects(const TaggedAST& label,
                                const Indexes<set<ObjectId>>& indexes) {
  const auto index_it = indexes.find(label.tag());
  if (index_it == indexes.end()) {
    return {};
  }
  const auto label_it = index_it->second.find(GetSerializationOrNull(label));
  if (label_it == index_it->second.end()) {
    return {};
  }
  return label_it->second;
}

}  // namespace

// Initialization creates indexes for each type of node and edge label. First,
// check if the contents of the maps 'node_types' and 'edge_types' are types.
// Then, create an empty index for each key value in 'node_types' and
// 'edge_types'. For non-unique label types, the index maps strings to sets of
// node or edge ids, and for unique label types, the index maps a string to a
// single node or edge id.
util::Status LabeledGraph::Initialize(Types node_types,
                                      const set<string>& unique_nodes,
                                      Types edge_types,
                                      const set<string>& unique_edges,
                                      AST graph_type) {
  string tmp_err;
  if (!type::AreTypes(node_types, &tmp_err)) {
    return util::Status(Code::INVALID_ARGUMENT,
                        util::StrCat("Type error in node_types:", tmp_err));
  }
  if (!type::AreTypes(edge_types, &tmp_err)) {
    return util::Status(Code::INVALID_ARGUMENT,
                        util::StrCat("Type error in edge_types:", tmp_err));
  }
  if (!type::IsType(graph_type, &tmp_err)) {
    return util::Status(Code::INVALID_ARGUMENT,
                        util::StrCat("Type error in graph_type:", tmp_err));
  }
  node_types_.swap(node_types);
  edge_types_.swap(edge_types);
  graph_type_.Swap(&graph_type);
  for (const string& tag : unique_nodes) {
    named_nodes_.insert({tag, Index<NodeId>()});
  }
  for (const auto& type : node_types_) {
    node_indexes_.insert({type.first, Index<set<NodeId>>()});
  }
  for (const string& tag : unique_edges) {
    named_edges_.insert({tag, EdgeIndex()});
  }
  for (const auto& type : edge_types_) {
    edge_indexes_.insert({type.first, Index<set<EdgeId>>()});
  }
  is_initialized_ = true;
  return util::Status::OK;
}

Types LabeledGraph::GetNodeTypes() const {
  CHECK(is_initialized_, kInitializationErr);
  return node_types_;
}

set<string> LabeledGraph::GetUniqueNodeTags() const {
  CHECK(is_initialized_, kInitializationErr);
  set<string> tags;
  // A 'tagged_index' is a pair consisting of a tag and an index.
  for (const auto& tagged_index : named_nodes_) {
    tags.insert(tagged_index.first);
  }
  return tags;
}

std::pair<bool, AST> LabeledGraph::GetNodeType(const string& tag) const {
  CHECK(is_initialized_, kInitializationErr);
  return GetTaggedType(tag, node_types_);
}

Types LabeledGraph::GetEdgeTypes() const {
  CHECK(is_initialized_, kInitializationErr);
  return edge_types_;
}

set<string> LabeledGraph::GetUniqueEdgeTags() const {
  CHECK(is_initialized_, kInitializationErr);
  set<string> tags;
  // A 'tagged_index' is a pair consisting of a tag and an index.
  for (const auto& tagged_index : named_edges_) {
    tags.insert(tagged_index.first);
  }
  return tags;
}

std::pair<bool, AST> LabeledGraph::GetEdgeType(const string& tag) const {
  CHECK(is_initialized_, kInitializationErr);
  return GetTaggedType(tag, edge_types_);
}

AST LabeledGraph::GetGraphType() const {
  CHECK(is_initialized_, kInitializationErr);
  return graph_type_;
}

void LabeledGraph::SetGraphLabel(AST graph_label) {
  CHECK(is_initialized_, kInitializationErr);
  string tmp_err;
  CHECK(type::IsTyped(graph_type_, graph_label, &tmp_err), tmp_err);
  graph_label_.Swap(&graph_label);
}

NodeId LabeledGraph::FindOrAddNode(const TaggedAST& label) {
  CHECK(is_initialized_, kInitializationErr);
  string tmp_err;
  CHECK(type::IsTyped(node_types_, label, &tmp_err), tmp_err);
  NodeId node_id;
  auto index_it = named_nodes_.find(label.tag());
  if (index_it == named_nodes_.end()) {
    node_id = InsertNode(label);
    IndexObject(label, node_id, &node_indexes_);
    return node_id;
  }
  string name = GetSerializationOrNull(label);
  Index<NodeId>& named_node = index_it->second;
  auto name_it = named_node.find(name);
  if (name_it == named_node.end()) {
    node_id = InsertNode(label);
    name_it = named_node.insert({name, node_id}).first;
  }
  return name_it->second;
}

util::Status LabeledGraph::UpdateNodeLabel(NodeId node_id,
                                           const TaggedAST& label) {
  CHECK(is_initialized_, kInitializationErr);
  string tmp_err;
  if (!type::IsTyped(node_types_, label, &tmp_err)) {
    return util::Status(Code::INVALID_ARGUMENT, tmp_err);
  }
  if (!HasNode(node_id)) {
    return util::Status(Code::INVALID_ARGUMENT, kInvalidNodeErr);
  }
  TaggedAST old_label = GetNodeLabel(node_id);
  // Update the label of the node and the relevant indexes.
  graph_[node_id] = label;
  if (IsUniqueNodeType(old_label)) {
    DeIndexUniqueNode(old_label, node_id, &named_nodes_);
  } else {
    DeIndexObject(old_label, node_id, &node_indexes_);
  }
  if (IsUniqueNodeType(label)) {
    return IndexUniqueNode(label, node_id, &named_nodes_);
  } else {
    return IndexObject(label, node_id, &node_indexes_);
  }
}

EdgeId LabeledGraph::FindOrAddEdge(NodeId source, NodeId target,
                                   const TaggedAST& label) {
  CHECK(is_initialized_, kInitializationErr);
  string tmp_err;
  CHECK(type::IsTyped(edge_types_, label, &tmp_err), tmp_err);
  EdgeId edge_id;
  auto index_it = named_edges_.find(label.tag());
  if (index_it == named_edges_.end()) {
    edge_id = InsertEdge(source, target, label);
    IndexObject(label, edge_id, &edge_indexes_);
    return edge_id;
  }
  EdgeIndex& named_edge = index_it->second;
  string name = GetSerializationOrNull(label);
  Edge edge(source, target, name);
  auto name_it = named_edge.find(edge);
  if (name_it == named_edge.end()) {
    edge_id = InsertEdge(source, target, label);
    name_it = named_edge.insert({Edge(source, target, name), edge_id}).first;
  }
  return name_it->second;
}

util::Status LabeledGraph::UpdateEdgeLabel(EdgeId edge_id,
                                           const TaggedAST& label) {
  CHECK(is_initialized_, kInitializationErr);
  string tmp_err;
  if (!type::IsTyped(edge_types_, label, &tmp_err)) {
    return util::Status(Code::INVALID_ARGUMENT, tmp_err);
  }
  if (!HasEdge(edge_id)) {
    return util::Status(Code::INVALID_ARGUMENT, kInvalidEdgeErr);
  }
  TaggedAST old_label = GetEdgeLabel(edge_id);
  // Update the label of the edge and the relevant indexes.
  graph_[edge_id] = label;
  if (IsUniqueEdgeType(old_label)) {
    string name = GetSerializationOrNull(old_label);
    Edge edge(Source(edge_id), Target(edge_id), name);
    DeIndexUniqueEdge(old_label.tag(), edge, &named_edges_);
  } else {
    DeIndexObject(old_label, edge_id, &edge_indexes_);
  }
  if (IsUniqueEdgeType(label)) {
    string name = GetSerializationOrNull(label);
    Edge edge(Source(edge_id), Target(edge_id), name);
    return IndexUniqueEdge(label.tag(), edge, edge_id, &named_edges_);
  } else {
    return IndexObject(label, edge_id, &edge_indexes_);
  }
}
// In a Boost adjacency list graph that uses vectors internally (like the
// LabeledGraph), node ids are unsigned values in the range [0, NumNodes() - 1],
// where NumNodes() is the number of nodes in the graph.
// http://www.boost.org/doc/libs/1_37_0/libs/graph/doc/adjacency_list.html
bool LabeledGraph::HasNode(NodeId node_id) const {
  return node_id < NumNodes();
}

// An EdgeId in Boost is implemented as a struct containing a source and a
// target node id, an edge type (directed or undirected), and a pointer to the
// edge label. This function checks if the source and target ids are valid and
// if there is indeed an edge between these nodes.
bool LabeledGraph::HasEdge(EdgeId edge_id) const {
  NodeId source_id = ::boost::source(edge_id, graph_);
  NodeId target_id = ::boost::target(edge_id, graph_);
  if (!HasNode(source_id) || !HasNode(target_id)) {
    return false;
  }
  return ::boost::edge(source_id, target_id, graph_).second;
}

// According to the Boost documentation, when a vertex is used to represent
// nodes (as done here), node ids range between 0 and the number of nodes in the
// graph.
// http://www.boost.org/doc/libs/1_37_0/libs/graph/doc/adjacency_list.html
TaggedAST LabeledGraph::GetNodeLabel(NodeId node_id) const {
  CHECK(is_initialized_, kInitializationErr);
  CHECK(HasNode(node_id), kInvalidNodeErr);
  return graph_[node_id];
}

TaggedAST LabeledGraph::GetEdgeLabel(EdgeId edge_id) const {
  CHECK(is_initialized_, kInitializationErr);
  CHECK(HasEdge(edge_id), kInvalidEdgeErr);
  return graph_[edge_id];
}

NodeId LabeledGraph::Source(EdgeId edge_id) const {
  CHECK(is_initialized_, kInitializationErr);
  CHECK(HasEdge(edge_id), kInvalidEdgeErr);
  return ::boost::source(edge_id, graph_);
}

NodeId LabeledGraph::Target(EdgeId edge_id) const {
  CHECK(is_initialized_, kInitializationErr);
  CHECK(HasEdge(edge_id), kInvalidEdgeErr);
  return ::boost::target(edge_id, graph_);
}

AST LabeledGraph::GetGraphLabel() const {
  CHECK(is_initialized_, kInitializationErr);
  return graph_label_;
}

bool LabeledGraph::IsUniqueNodeType(const TaggedAST& label_type) const {
  CHECK(is_initialized_, kInitializationErr);
  return (named_nodes_.find(label_type.tag()) != named_nodes_.end());
}

bool LabeledGraph::IsUniqueEdgeType(const TaggedAST& label_type) const {
  CHECK(is_initialized_, kInitializationErr);
  return (named_edges_.find(label_type.tag()) != named_edges_.end());
}

set<NodeId> LabeledGraph::GetNodes(const TaggedAST& label) const {
  CHECK(is_initialized_, kInitializationErr);
  const auto index_it = named_nodes_.find(label.tag());
  if (index_it == named_nodes_.end()) {
    return GetLabeledObjects(label, node_indexes_);
  }
  const Index<NodeId>& named_node = index_it->second;
  const auto name_it = named_node.find(GetSerializationOrNull(label));
  if (name_it == named_node.end()) {
    return {};
  }
  return {name_it->second};
}

set<EdgeId> LabeledGraph::GetEdges(const TaggedAST& label) const {
  CHECK(is_initialized_, kInitializationErr);
  const auto index_it = named_edges_.find(label.tag());
  if (index_it == named_edges_.end()) {
    return GetLabeledObjects(label, edge_indexes_);
  }
  const EdgeIndex& edge_index = index_it->second;
  const string& name = GetSerializationOrNull(label);
  set<EdgeId> edges;
  for (const auto& key_edge : edge_index) {
    if (key_edge.first.label == name) {
      edges.insert(key_edge.second);
    }
  }
  return edges;
}

// In a Boost graph which uses an adjacency list representation, the type NodeId
// is an unsigned int and valid node_ids are in the range [0, NumNodes() - 1].
set<NodeId> LabeledGraph::GetPredecessors(NodeId node_id) const {
  CHECK(is_initialized_, kInitializationErr);
  CHECK(HasNode(node_id), kInvalidNodeErr);
  set<NodeId> predecessors;
  for (InEdgeIterator edge_it = InEdgeBegin(node_id);
       edge_it != InEdgeEnd(node_id); ++edge_it) {
    predecessors.insert(Source(*edge_it));
  }
  return predecessors;
}

set<NodeId> LabeledGraph::GetSuccessors(NodeId node_id) const {
  CHECK(is_initialized_, kInitializationErr);
  CHECK(HasNode(node_id), kInvalidNodeErr);
  set<NodeId> successors;
  for (OutEdgeIterator edge_it = OutEdgeBegin(node_id);
       edge_it != OutEdgeEnd(node_id); ++edge_it) {
    successors.insert(Target(*edge_it));
  }
  return successors;
}

InEdgeIterator LabeledGraph::InEdgeBegin(NodeId node_id) const {
  CHECK(is_initialized_, kInitializationErr);
  CHECK(HasNode(node_id), kInvalidNodeErr);
  return ::boost::in_edges(node_id, graph_).first;
}

InEdgeIterator LabeledGraph::InEdgeEnd(NodeId node_id) const {
  CHECK(is_initialized_, kInitializationErr);
  CHECK(HasNode(node_id), kInvalidNodeErr);
  return ::boost::in_edges(node_id, graph_).second;
}

OutEdgeIterator LabeledGraph::OutEdgeBegin(NodeId node_id) const {
  CHECK(is_initialized_, kInitializationErr);
  CHECK(HasNode(node_id), kInvalidNodeErr);
  return ::boost::out_edges(node_id, graph_).first;
}

OutEdgeIterator LabeledGraph::OutEdgeEnd(NodeId node_id) const {
  CHECK(is_initialized_, kInitializationErr);
  CHECK(HasNode(node_id), kInvalidNodeErr);
  return ::boost::out_edges(node_id, graph_).second;
}

set<NodeId> LabeledGraph::GetLabelPredecessors(const TaggedAST& label) const {
  CHECK(is_initialized_, kInitializationErr);
  set<NodeId> target_nodes = GetNodes(label);
  set<NodeId> predecessors;
  set<NodeId> sources;
  for (NodeId target_id : target_nodes) {
    sources = GetPredecessors(target_id);
    predecessors.insert(sources.begin(), sources.end());
  }
  return predecessors;
}

set<NodeId> LabeledGraph::GetLabelSuccessors(const TaggedAST& label) const {
  CHECK(is_initialized_, kInitializationErr);
  set<NodeId> source_nodes = GetNodes(label);
  set<NodeId> successors;
  set<NodeId> targets;
  for (NodeId source_id : source_nodes) {
    targets = GetSuccessors(source_id);
    successors.insert(targets.begin(), targets.end());
  }
  return successors;
}

NodeIterator LabeledGraph::NodeSetBegin() const {
  CHECK(is_initialized_, kInitializationErr);
  return ::boost::vertices(graph_).first;
}

NodeIterator LabeledGraph::NodeSetEnd() const {
  CHECK(is_initialized_, kInitializationErr);
  return ::boost::vertices(graph_).second;
}

EdgeIterator LabeledGraph::EdgeSetBegin() const {
  CHECK(is_initialized_, kInitializationErr);
  return ::boost::edges(graph_).first;
}

EdgeIterator LabeledGraph::EdgeSetEnd() const {
  CHECK(is_initialized_, kInitializationErr);
  return ::boost::edges(graph_).second;
}

int LabeledGraph::NumNodeTypes() const {
  CHECK(is_initialized_, kInitializationErr);
  return node_types_.size();
}

int LabeledGraph::NumUniqueNodeTypes() const {
  CHECK(is_initialized_, kInitializationErr);
  return named_nodes_.size();
}

int LabeledGraph::NumNodes() const {
  CHECK(is_initialized_, kInitializationErr);
  return ::boost::num_vertices(graph_);
}

int LabeledGraph::NumLabeledNodes(const TaggedAST& label) const {
  CHECK(is_initialized_, kInitializationErr);
  return GetNodes(label).size();
}

int LabeledGraph::NumEdgeTypes() const {
  CHECK(is_initialized_, kInitializationErr);
  return edge_types_.size();
}

int LabeledGraph::NumUniqueEdgeTypes() const {
  CHECK(is_initialized_, kInitializationErr);
  return named_edges_.size();
}

int LabeledGraph::NumEdges() const {
  CHECK(is_initialized_, kInitializationErr);
  return ::boost::num_edges(graph_);
}

int LabeledGraph::NumLabeledEdges(const TaggedAST& label) const {
  CHECK(is_initialized_, kInitializationErr);
  return GetEdges(label).size();
}

NodeId LabeledGraph::InsertNode(TaggedAST label) {
  NodeId node_id = ::boost::add_vertex(graph_);
  graph_[node_id].Swap(&label);
  return node_id;
}

// ::boost::add_edge(..) adds an edge from a source to a target node and returns
// a pair. The first element of the pair is an edge id and the second is a bool
// whose value is relevant for graphs in which there can be at most one edge
// between two vertices. Uniqueness in LabeledGraph depends on labels so the
// bool value is ignored here.
EdgeId LabeledGraph::InsertEdge(NodeId source, NodeId target, TaggedAST label) {
  EdgeId edge_id = ::boost::add_edge(source, target, graph_).first;
  graph_[edge_id].Swap(&label);
  return edge_id;
}

}  // namespace third_party_logle
