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

#include "third_party/logle/graph_transformer.h"
#include "third_party/logle/type.h"
#include "third_party/logle/util/logging.h"
#include "third_party/logle/util/status.h"
#include "third_party/logle/util/string_utils.h"
#include "third_party/logle/value.h"

namespace third_party_logle {

using ast::type::Types;
namespace type = ast::type;

namespace {

const char* const kBlockTag = "Block";
const char* const kEdgeTag = "Edge";
const char* const kQuotientGraphTag = "Quotient-Graph";
const char* const kSubNode = "Sub-Node";

// A Transformation consists of a reference to an input graph, an output graph
// and a map between the nodes of the input and the output graph. A
// transformation is used for implementation convenience to encapsulate the data
// accessed by helper functions used by graph transformations. A transformation
// is represented as a struct and not a class because a function implementing a
// transformation needs full access to the API of the input graph, the node map
// and the output graph.
//
// TODO(vijaydsilva): Revisit whether transformation should be upgraded to a
// class with member functions or eliminated entirely after implementing more
// transformations.
struct Transformation {
 public:
  explicit Transformation(const LabeledGraph& graph) : input(graph) {}

  const LabeledGraph& input;
  map<NodeId, NodeId> node_map;
  std::unique_ptr<LabeledGraph> output;
};  // struct Transformation

// The type of a graph is defined by the type of node and edge labels the graph
// can have and the type of the graph label.
//
// Returns a pointer to a graph whose type is the same as the input graph.
// Returns a nullptr if a new graph could not be created. The returned graph
// will be empty (no nodes and no edges) because only the type of the graph is
// copied, not the contents of the graph.
std::unique_ptr<LabeledGraph> CloneGraphType(const LabeledGraph& graph) {
  std::unique_ptr<LabeledGraph> new_graph(new LabeledGraph());
  util::Status status = new_graph->Initialize(
      graph.GetNodeTypes(), graph.GetUniqueNodeTags(), graph.GetEdgeTypes(),
      graph.GetUniqueEdgeTags(), graph.GetGraphType());
  if (!status.ok()) {
    new_graph.reset(nullptr);
  }
  return new_graph;
}

// The graph transformer maintains a map from node identifiers in the input
// graph to identifiers in the transformed graph. This function returns the
// identifier in the transformed graph corresponding to the argument 'node_id'
// if an entry exists in the map. If no such entry exists, a node corresponding
// to 'node_id' is created in the transformed graph with the label 'new_label'
// and the identifier of this new node is returned.
NodeId FindOrRelabelNode(NodeId node_id, TaggedAST new_label,
                         Transformation* transform) {
  NodeId new_node;
  map<NodeId, NodeId>& node_map = transform->node_map;
  auto map_it = node_map.find(node_id);
  if (map_it == node_map.end()) {
    new_node = transform->output->FindOrAddNode(new_label);
    node_map.insert({node_id, new_node});
  } else {
    new_node = map_it->second;
  }
  return new_node;
}

// Takes an integer and returns a block label with that integer as the AST
// payload and the tag.
TaggedAST MakeBlockLabel(int label) {
  TaggedAST tast;
  *tast.mutable_ast() = ast::value::MakeInt(label);
  tast.set_tag(kBlockTag);
  return tast;
}

// Takes an id and adds the corresponding block to transform->output as dictated
// by 'partition'. 'blocks' is used to keep track of duplicate blocks, as at the
// moment FindOrAddNode does not check for duplicates as needed here. b/29351440
//
// Returns the id of the corresponding block node in the output graph.
NodeId FindOrAddBlock(NodeId node_id, const std::map<NodeId, int>& partition,
               std::map<int, NodeId>* blocks,
               Transformation* transform) {
    const auto node_it = partition.find(node_id);
    CHECK(node_it != partition.end(),
          util::StrCat("The following node is missing from the partition: ",
                       std::to_string(node_id)));

    int block_num = node_it->second;
    // If the corresponding block is not in the graph, add it.
    auto block_it = blocks->find(block_num);
    NodeId block_id;
    if (block_it == blocks->end()) {
      TaggedAST block_label = MakeBlockLabel(block_num);
      block_id = transform->output->FindOrAddNode(block_label);
      (*blocks)[block_num] = block_id;
    } else {
      block_id = block_it->second;
    }
    return block_id;
}

std::unique_ptr<LabeledGraph> MakeBlockGraph() {
  std::unique_ptr<LabeledGraph> new_graph(new LabeledGraph());

  type::Types node_types;
  node_types.emplace(kBlockTag,
                     type::MakeInt(kBlockTag, /*Must not be null*/ false));
  type::Types edge_types;
  edge_types.emplace(kEdgeTag,
                     type::MakeInt(kEdgeTag, /*Must not be null*/ false));
  AST graph_type = type::MakeNull(kQuotientGraphTag);
  new_graph->Initialize(node_types, {}, edge_types, {}, graph_type);

  return new_graph;
}

}  // namespace

namespace graph {

// The deletion function iterates over nodes in the input graph and over the
// outgoing edges from each node. An edge is copied to the transformed graph if
// neither the source nor the target of the edge is a node to be deleted. This
// function uses a helper function FindOrRelabelNode but not a helper function
// FindOrRelabelEdge because edges in the input graph are traversed in order of
// their source nodes. A FindOrRelabelEdge function would not know that the
// source node of an edge in a sequence of calls is the same and would perform
// redudnant lookups in the node map.
std::unique_ptr<LabeledGraph> DeleteNodes(const LabeledGraph& graph,
                                          const set<NodeId>& nodes) {
  Transformation transform(graph);
  transform.output = CloneGraphType(graph);
  if (transform.output == nullptr) {
    return std::move(transform.output);
  }

  NodeIterator end_it = graph.NodeSetEnd();
  for (NodeIterator node_it = graph.NodeSetBegin(); node_it != end_it;
       ++node_it) {
    NodeId src = *node_it;
    if (nodes.find(src) != nodes.end()) {
      continue;
    }
    NodeId new_src =
        FindOrRelabelNode(src, graph.GetNodeLabel(src), &transform);
    OutEdgeIterator out_edge_end = graph.OutEdgeEnd(src);
    for (OutEdgeIterator edge_it = graph.OutEdgeBegin(src);
         edge_it != out_edge_end; ++edge_it) {
      NodeId tgt = graph.Target(*edge_it);
      if (nodes.find(tgt) != nodes.end()) {
        continue;
      }
      NodeId new_tgt =
          FindOrRelabelNode(tgt, graph.GetNodeLabel(tgt), &transform);
      transform.output->FindOrAddEdge(new_src, new_tgt,
                                      graph.GetEdgeLabel(*edge_it));
    }
  }
  return std::move(transform.output);
}


std::unique_ptr<LabeledGraph> QuotientGraph(
                              const LabeledGraph& graph,
                              const std::map<NodeId, int>& partition) {
  Transformation transform(graph);
  transform.output = MakeBlockGraph();
  if (transform.output == nullptr) {
    return std::move(transform.output);
  }
  std::map<int, NodeId> blocks;
  NodeIterator node_end_it = graph.NodeSetEnd();
  for (NodeIterator node_it = graph.NodeSetBegin(); node_it != node_end_it;
       ++node_it) {
    NodeId src = *node_it;
    NodeId src_block = FindOrAddBlock(src, partition, &blocks, &transform);
    OutEdgeIterator edge_end_it = graph.OutEdgeEnd(src);
    for (OutEdgeIterator edge_it = graph.OutEdgeBegin(src);
         edge_it != edge_end_it; ++edge_it) {
      NodeId tgt = graph.Target(*edge_it);
      NodeId tgt_block = FindOrAddBlock(tgt, partition, &blocks, &transform);
      TaggedAST new_edge_label(graph.GetEdgeLabel(*edge_it));
      new_edge_label.set_tag(kEdgeTag);
      transform.output->FindOrAddEdge(src_block, tgt_block, new_edge_label);
    }
  }
  return std::move(transform.output);
}

}  // namespace graph
}  // namespace third_party_logle
