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

#include "graph_transformer.h"

#include <queue>

#include "type.h"
#include "util/logging.h"
#include "util/status.h"
#include "util/string_utils.h"
#include "value.h"

namespace morphie {

using ast::type::Types;
namespace type = ast::type;

namespace {

using QuotientEdgeMap = std::map<std::pair<NodeId, NodeId>, std::set<EdgeId>>;

// This map keeps track of all of the predecessors and successors for each node
// that is being folded. The ordering in the pair is predecessors then
// successors.
using AdjMap = std::map<NodeId, std::pair<std::set<NodeId>, std::set<NodeId>>>;

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
  std::map<NodeId, NodeId> node_map;
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
  std::map<NodeId, NodeId>& node_map = transform->node_map;
  auto map_it = node_map.find(node_id);
  if (map_it == node_map.end()) {
    new_node = transform->output->FindOrAddNode(new_label);
    node_map.insert({node_id, new_node});
  } else {
    new_node = map_it->second;
  }
  return new_node;
}

void BuildQuotientEdgeMap(const graph::QuotientConfig& config,
                          const std::map<NodeId, int>& partition,
                          std::map<int, NodeId>* blocks,
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
    // Do not include self-edges if they are not allowed.
    if (!config.allow_self_edges && src_block == tgt_block) {
      continue;
    }
    std::pair<NodeId, NodeId> quotient_edge(src_block, tgt_block);
    auto member_it = block_edge_members->find(quotient_edge);
    if (member_it == block_edge_members->end()) {
      block_edge_members->insert({quotient_edge, {*edge_it}});
    } else {
      member_it->second.insert(*edge_it);
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
    auto labels = edge_label_fn(transform->input, member_it->second);
    for (auto edge_label : labels) {
      transform->output->FindOrAddEdge(src, tgt, edge_label);
    }
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

// Returns a map from each node to its adjacent nodes in the 'edges' relation.
std::map<NodeId, std::set<NodeId>> MakeAdjacencyMap(
    const LabeledGraph& graph, const std::set<EdgeId>& edges) {
  std::map<NodeId, std::set<NodeId>> adj_map;
  for (const auto& edge : edges) {
    NodeId src = graph.Source(edge);
    NodeId tgt = graph.Target(edge);
    auto src_it = adj_map.find(src);
    if (src_it == adj_map.end()) {
      adj_map[src] = {tgt};
    } else {
      src_it->second.insert(tgt);
    }
    auto tgt_it = adj_map.find(tgt);
    if (tgt_it == adj_map.end()) {
      adj_map[tgt] = {src};
    } else {
      tgt_it->second.insert(src);
    }
  }
  return adj_map;
}

// Returns a partition where each block is a connected component under the
// relation 'edges'. The label of each block ranges from 0 to b-1 where b is the
// number of blocks.
std::map<NodeId, int> MakePartitionFromRelation(
    const LabeledGraph& graph,
    const std::map<NodeId, std::set<NodeId>>& adj_map) {
  std::map <NodeId, int> partition;
  // Go through each node in 'graph' and do a BFS to assign it to some block. A
  // node is assigned a block by assigning it a value in the 'partition' map.
  int current_block_id = 0;
  auto end_it = graph.NodeSetEnd();
  for (auto node_it = graph.NodeSetBegin(); node_it != end_it; ++node_it) {
    auto part_it = partition.find(*node_it);
    if (part_it != partition.end()) {
      continue;
    }
    std::queue<NodeId> frontier;
    frontier.push(*node_it);
    while (!frontier.empty()) {
      NodeId node = frontier.front();
      frontier.pop();
      // Check if the node has already been assigned a block. If so, then there
      // is not need to process it again.
      part_it = partition.find(node);
      if (part_it != partition.end()) {
        continue;
      }
      partition[node] = current_block_id;
      // Check that the current node has neighbors.
      auto adj_it = adj_map.find(node);
      if (adj_it == adj_map.end()) {
        continue;
      }
      const auto& adjacent_nodes = adj_it->second;
      for (auto& node : adjacent_nodes) {
        frontier.push(node);
      }
    }
    ++current_block_id;
  }
  return partition;
}

// For each node that will be folded, the initial predecessors and successors
// are just their predecessors and successors in the graph.
AdjMap CreateAdjMap(const LabeledGraph& graph, const std::set<NodeId>& nodes) {
  AdjMap adj_map;
  for (NodeId node : nodes) {
    std::set<NodeId> predecessors(graph.GetPredecessors(node));
    std::set<NodeId> successors(graph.GetSuccessors(node));
    predecessors.erase(node);
    successors.erase(node);
    adj_map.insert({node, {predecessors, successors}});
  }
  return adj_map;
}

// Replaces 'node' in the output graph of 'transform' with a biparatite graph
// between its predecessors and successors. For each of its neighbors that is
// also going to be folded, instead of adding an edge it updates their
// predecessor/successor set in the 'adj_map'.
void ReplaceNodeWithBipartite(const LabeledGraph& graph,
                              const graph::FoldLabelFn& fold_label_fn,
                              NodeId node, AdjMap* adj_map,
                              Transformation* transform) {
  auto neighbors = adj_map->find(node)->second;
  std::set<NodeId> predecessors = neighbors.first;
  std::set<NodeId> successors = neighbors.second;
  for (NodeId predecessor : predecessors) {
    auto pred_map_it = adj_map->find(predecessor);
    // Make an edge if both predecessor and successor are not being folded.
    bool make_edge = true;
    // The set of successors for 'predecessor' in the map. nullptr if
    // 'predecessor' is not in 'adj_map'.
    std::set<NodeId>* pred_succ_set = nullptr;
    if (pred_map_it != adj_map->end()) {
      pred_succ_set = &(pred_map_it->second.second);
      make_edge = false;
    }
    for (NodeId successor : successors) {
      if (pred_succ_set != nullptr) {
        pred_succ_set->insert(successor);
      }
      auto succ_map_it = adj_map->find(successor);
      if (succ_map_it != adj_map->end()) {
        succ_map_it->second.first.insert(predecessor);
        make_edge = false;
      }
      if (!make_edge) {
        continue;
      }
      std::vector<TaggedAST> labels = fold_label_fn(graph, node,
                                                    predecessor, successor);
      NodeId new_pred = FindOrRelabelNode(predecessor,
                                          graph.GetNodeLabel(predecessor),
                                          transform);
      NodeId new_succ = FindOrRelabelNode(successor,
                                          graph.GetNodeLabel(successor),
                                          transform);
      for (auto label : labels) {
        transform->output->FindOrAddEdge(new_pred, new_succ, label);
      }
    }
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
std::unique_ptr<Morphism> DeleteNodes(const LabeledGraph& graph,
                                      const std::set<NodeId>& nodes) {
  std::unique_ptr<Morphism> morphism(new Morphism(&graph));
  morphism->CopyInputType();
  if (!morphism->HasOutputGraph()) {
    return morphism;
  }
  NodeIterator end_it = graph.NodeSetEnd();
  for (NodeIterator node_it = graph.NodeSetBegin(); node_it != end_it;
       ++node_it) {
    NodeId src = *node_it;
    // If this node is to be deleted, skip it.
    if (nodes.find(src) != nodes.end()) {
      continue;
    }
    morphism->FindOrCopyNode(src);
    OutEdgeIterator out_edge_end = graph.OutEdgeEnd(src);
    for (OutEdgeIterator edge_it = graph.OutEdgeBegin(src);
         edge_it != out_edge_end; ++edge_it) {
      NodeId tgt = graph.Target(*edge_it);
      // If the target node is to be deleted, skip that node and this edge.
      if (nodes.find(tgt) != nodes.end()) {
        continue;
      }
      morphism->FindOrCopyEdge(*edge_it);
    }
  }
  return morphism;
}

// The deletion function iterates over nodes in the input graph and copies the
// source and target node of each edge to the new graph. An edge is copied to
// the new graph if the edge is not in the set of edges to delete.
std::unique_ptr<Morphism> DeleteEdgesNotNodes(const LabeledGraph& graph,
                                              const std::set<EdgeId>& edges) {
  std::unique_ptr<Morphism> morphism(new Morphism(&graph));
  morphism->CopyInputType();
  if (!morphism->HasOutputGraph()) {
    return morphism;
  }
  NodeIterator end_it = graph.NodeSetEnd();
  for (NodeIterator node_it = graph.NodeSetBegin(); node_it != end_it;
       ++node_it) {
    NodeId src = *node_it;
    morphism->FindOrCopyNode(src);
    OutEdgeIterator out_edge_end = graph.OutEdgeEnd(src);
    for (OutEdgeIterator edge_it = graph.OutEdgeBegin(src);
         edge_it != out_edge_end; ++edge_it) {
      NodeId tgt = graph.Target(*edge_it);
      morphism->FindOrCopyNode(tgt);
      // Skip the edge if it is to be deleted.
      if (edges.find(*edge_it) != edges.end()) {
        continue;
      }
      morphism->FindOrCopyEdge(*edge_it);
    }
  }
  return morphism;
}

// The deletion function iterates over edges (not nodes) in the input graph and
// copies edges that are not to be deleted to the output graph. A node that has
// no incident edges in the original graph or whose incident edges ae all in the
// deletion set will not be added to the new graph.
std::unique_ptr<Morphism> DeleteEdgesAndNodes(const LabeledGraph& graph,
                                              const std::set<EdgeId>& edges) {
  std::unique_ptr<Morphism> morphism(new Morphism(&graph));
  morphism->CopyInputType();
  if (!morphism->HasOutputGraph()) {
    return morphism;
  }
  EdgeIterator end_it = graph.EdgeSetEnd();
  for (EdgeIterator edge_it = graph.EdgeSetBegin(); edge_it != end_it;
       ++edge_it) {
    if (edges.find(*edge_it) != edges.end()) {
      continue;
    }
    morphism->FindOrCopyEdge(*edge_it);
  }
  return morphism;
}

std::unique_ptr<LabeledGraph> QuotientGraph(
    const LabeledGraph& input_graph, const std::map<NodeId, int>& partition,
    const QuotientConfig& config) {
  Transformation transform(input_graph);
  transform.output = CloneGraphType(config.output_graph_type);
  if (transform.output == nullptr) {
    return std::move(transform.output);
  }
  std::map<int, std::set<NodeId>> block_members;
  BuildQuotientNodeMap(partition, transform, &block_members);
  // Add blocks to the output graph with the label generated by 'node_label_fn'.
  std::map<int, NodeId> block_node_ids;
  AddQuotientNodes(config.node_label_fn, block_members,
                   &block_node_ids, &transform);

  std::map<std::pair<NodeId, NodeId>, std::set<EdgeId>> block_edge_members;
  BuildQuotientEdgeMap(config, partition,
                       &block_node_ids, &block_edge_members, &transform);
  AddQuotientEdges(block_edge_members, config.edge_label_fn, &transform);
  return std::move(transform.output);
}

std::unique_ptr<LabeledGraph> ContractEdges(const LabeledGraph& graph,
                                            const std::set<EdgeId>& edges,
                                            const QuotientConfig& config) {
  std::map<NodeId, std::set<NodeId>> adj_map = MakeAdjacencyMap(graph, edges);
  std::map<NodeId, int> partition = MakePartitionFromRelation(graph, adj_map);
  std::unique_ptr<Morphism> morphism = DeleteEdgesNotNodes(graph, edges);
  return QuotientGraph(morphism->Output(), partition, config);
}

// The FoldNodes function iterates over the nodeset of 'graph' and does one
// of two things. If the node is in the set 'nodes', then
// ReplaceNodeWithBipartite is called on the node. Otherwise we iterate over the
// outgoing edges, adding all neighbors not in 'nodes' and edges to those
// neighbors.
std::unique_ptr<LabeledGraph> FoldNodes(const LabeledGraph& graph,
                                        const FoldLabelFn& fold_label_fn,
                                        const std::set<NodeId>& nodes) {
  Transformation transform(graph);
  transform.output = CloneGraphType(graph);
  if (transform.output == nullptr) {
    return std::move(transform.output);
  }
  AdjMap adj_map = CreateAdjMap(graph, nodes);

  NodeIterator end_it = graph.NodeSetEnd();
  for (NodeIterator node_it = graph.NodeSetBegin(); node_it != end_it;
       ++node_it) {
    NodeId src = *node_it;
    if (nodes.find(src) != nodes.end()) {
      ReplaceNodeWithBipartite(graph, fold_label_fn, src,
                               &adj_map, &transform);
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
}  // namespace graph
}  // namespace morphie
