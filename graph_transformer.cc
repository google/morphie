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

#include "third_party/logle/util/status.h"

namespace third_party_logle {
namespace {

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

}  // namespace graph
}  // namespace third_party_logle
