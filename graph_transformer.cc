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

using QuotientEdgeMap = std::map<std::pair<NodeId, NodeId>, std::set<EdgeId>>;

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

void BuildQuotientEdgeMap(const std::map<NodeId, int>& partition,
                          bool allow_multi_edges, std::map<int, NodeId>* blocks,
                          QuotientEdgeMap* block_edge_members,
                          Transformation* transform) {
  const LabeledGraph& graph = transform->input;
  EdgeIterator end_it = graph.EdgeSetEnd();
  for (EdgeIterator edge_it = graph.EdgeSetBegin(); edge_it != end_it;
       ++edge_it) {
    NodeId src = graph.Source(*edge_it);
    NodeId src_block = blocks->at(partition.at(src));
    NodeId tgt = graph.Target(*edge_it);
    NodeId tgt_block = blocks->at(partition.at(tgt));
    // If multi-edges are allowed the FindOrAddEdge method is used to add
    // multi-edges. Otherwise 'block_edge_members' records the EdgeId's of the
    // edges from 'src_block' to 'tgt_block'. The edges are added to the graph
    // at the end.
    if (allow_multi_edges) {
      transform->output->FindOrAddEdge(src_block, tgt_block,
                                       graph.GetEdgeLabel(*edge_it));
    } else {
      std::pair<NodeId, NodeId> quotient_edge(src_block, tgt_block);
      auto member_it = block_edge_members->find(quotient_edge);
      if (member_it == block_edge_members->end()) {
        block_edge_members->insert({quotient_edge, {*edge_it}});
      } else {
        member_it->second.insert(*edge_it);
      }
    }
  }
}

// Add the relabeled, collapsed edges to the graph.
void AddQuotientEdges(const QuotientEdgeMap& block_edge_members,
                      const graph::EdgeLabelFn& edge_label_fn,
                      Transformation* transform) {
  auto member_end_it = block_edge_members.end();
  for (auto member_it = block_edge_members.begin(); member_it != member_end_it;
       ++member_it) {
    auto edge_pair = member_it->first;
    NodeId src = edge_pair.first;
    NodeId tgt = edge_pair.second;
    TaggedAST edge_tag = edge_label_fn(transform->input, member_it->second);
    transform->output->FindOrAddEdge(src, tgt, edge_tag);
  }
}

void BuildQuotientNodeMap(const std::map<NodeId, int>& partition,
                          const Transformation& transform,
                          std::map<int, std::set<NodeId>>* block_members) {
  const LabeledGraph& input_graph = transform.input;
  NodeIterator node_end_it = input_graph.NodeSetEnd();
  for (NodeIterator node_it = input_graph.NodeSetBegin();
       node_it != node_end_it; ++node_it) {
    NodeId src = *node_it;
    // Keep track of all of the nodes in each block. The set of nodes will be
    // passed to 'node_label_fn' to properly label each block node.
    const auto partition_it = partition.find(src);
    CHECK(partition_it != partition.end(),
          util::StrCat("The following node is missing from the partition: ",
                       std::to_string(src)));
    int block_id = partition_it->second;
    auto block_it = block_members->find(block_id);
    if (block_it == block_members->end()) {
      block_members->insert({block_id, {src}});
    } else {
      block_it->second.insert(src);
    }
  }
}

void AddQuotientNodes(const graph::NodeLabelFn& node_label_fn,
                      const std::map<int, std::set<NodeId>>& block_members,
                      std::map<int, NodeId>* block_node_ids,
                      Transformation* transform) {
  auto block_end_it = block_members.end();
  for (auto block_it = block_members.begin(); block_it != block_end_it;
       ++block_it) {
    int block_id = block_it->first;
    TaggedAST block_label = node_label_fn(transform->input, block_it->second);
    NodeId block_node_id = transform->output->FindOrAddNode(block_label);
    block_node_ids->insert({block_id, block_node_id});
  }
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

// The deletion function iterates over edges in the input graph and copies the
// source and target node of each edge to the new graph. An edge is copied to
// the new graph if the edge is not in the set of edges to delete.
std::unique_ptr<LabeledGraph> DeleteEdges(const LabeledGraph& graph,
                                          const set<EdgeId>& edges) {
  Transformation transform(graph);
  transform.output = CloneGraphType(graph);
  if (transform.output == nullptr) {
    return std::move(transform.output);
  }
  EdgeIterator end_it = graph.EdgeSetEnd();
  for (EdgeIterator edge_it = graph.EdgeSetBegin(); edge_it != end_it;
       ++edge_it) {
    EdgeId old_edge = *edge_it;
    NodeId old_src = graph.Source(*edge_it);
    NodeId old_tgt = graph.Target(*edge_it);
    NodeId new_src =
        FindOrRelabelNode(old_src, graph.GetNodeLabel(old_src), &transform);
    NodeId new_tgt =
        FindOrRelabelNode(old_tgt, graph.GetNodeLabel(old_tgt), &transform);
    if (edges.find(old_edge) != edges.end()) {
      continue;
    }
    transform.output->FindOrAddEdge(new_src, new_tgt,
                                    graph.GetEdgeLabel(*edge_it));
  }
  return std::move(transform.output);
}

std::unique_ptr<LabeledGraph> QuotientGraph(
    const LabeledGraph& input_graph, const LabeledGraph& output_graph_type,
    const std::map<NodeId, int>& partition, const NodeLabelFn& node_label_fn,
    const EdgeLabelFn& edge_label_fn, bool allow_multi_edges) {
  Transformation transform(input_graph);
  transform.output = CloneGraphType(output_graph_type);
  if (transform.output == nullptr) {
    return std::move(transform.output);
  }
  std::map<int, std::set<NodeId>> block_members;
  BuildQuotientNodeMap(partition, transform, &block_members);
  // Add blocks to the output graph with the label generated by 'node_label_fn'.
  std::map<int, NodeId> block_node_ids;
  AddQuotientNodes(node_label_fn, block_members, &block_node_ids, &transform);

  std::map<std::pair<NodeId, NodeId>, std::set<EdgeId>> block_edge_members;
  BuildQuotientEdgeMap(partition, allow_multi_edges, &block_node_ids,
                       &block_edge_members, &transform);
  // If multi-edges are not allowed, add edges from 'block_edge_members' to the
  // graph.
  if (!allow_multi_edges) {
    AddQuotientEdges(block_edge_members, edge_label_fn, &transform);
  }
  return std::move(transform.output);
}

}  // namespace graph
}  // namespace third_party_logle
