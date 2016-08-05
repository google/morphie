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

// A labeled graph can be transformed by deleting or collapsing nodes and edges.
// This file contains utilities for transforming graphs.
#ifndef THIRD_PARTY_LOGLE_GRAPH_TRANSFORMER_H_
#define THIRD_PARTY_LOGLE_GRAPH_TRANSFORMER_H_

#include <memory>
#include <set>

#include "labeled_graph.h"

namespace tervuren {
// The 'graph' namespace contains functions for manipulating graphs.
namespace graph {

// A NodeLabelFn takes in a set of nodes in a graph and generates a new label.
using NodeLabelFn =
    std::function<TaggedAST(const LabeledGraph&, const std::set<NodeId>&)>;

// A EdgeLabelFn takes as input a set of edges in a graph and generates a new
// label.
using EdgeLabelFn =
    std::function<TaggedAST(const LabeledGraph&, const std::set<EdgeId>&)>;

// A FoldLabelFn takes three nodes in a graph and generates a label for the new
// edge to be generated. If we have nodes A -> B -> C where B is being folded,
// the ordering to used is (B, A, C) as input to the function.
using FoldLabelFn =
    std::function<TaggedAST(const LabeledGraph&, NodeId, NodeId, NodeId)>;

// Struct to hold options data for QuotientGraph:
// - The 'output_graph_type' specifies the types of nodes and edges permitted in
//   the output graph.
// - The 'node_label_fn' determines how the blocks are labeled in the output.
// - The 'edge_label_fn' determines how the edges between the blocks are
//   labeled. This function is only applied if 'allow_multi_edges' is false.
// - The flag 'allow_multi_edges' dictates whether the output graph will allow
//    multi-edges between nodes, or will instead have a single edge, labeled by
//    'edge_label_fn'.
// - The flag 'allow_self_edges' dictates if the output graph should contain
//    self-edges.
//
// Requires that:
// - Both 'node_label_fn' and 'edge_label_fn' respect the types of
//   'output_graph_type'.
// - If 'allow_multi_edges' is true, the edge types of 'input_graph' and
//   'output_graph_type' must be the same.
struct QuotientConfig {
 public:
  explicit QuotientConfig(const LabeledGraph& output_graph_type,
                          const NodeLabelFn& node_label_fn,
                          const EdgeLabelFn& edge_label_fn,
                          bool allow_multi_edges, bool allow_self_edges)
      : output_graph_type(output_graph_type),
      node_label_fn(node_label_fn),
      edge_label_fn(edge_label_fn),
      allow_multi_edges(allow_multi_edges),
      allow_self_edges(allow_self_edges) {}
  const LabeledGraph& output_graph_type;
  const NodeLabelFn& node_label_fn;
  const EdgeLabelFn& edge_label_fn;
  bool allow_multi_edges;
  bool allow_self_edges;
};  // struct QuotientConfig

// If G = (V, E) is a graph and N is a subset of nodes of V, the result of
// deleting N from G is the graph with nodes W = (V - N) and with those edges in
// E whose source and target are both in W.
//
// Returns a labeled graph obtained by deleting 'nodes' from the input graph.
// Has time complexity linear in the number of edges of the input graph and
// space complexity linear in the number of nodes and edges of the ouput graph.
// - Requires that 'nodes' contains valid node identifiers of the input graph.
std::unique_ptr<LabeledGraph> DeleteNodes(const LabeledGraph& graph,
                                          const set<NodeId>& nodes);

// If G = (V, E) is a graph and F is a subset of edges of E, the result of
// deleting F from G is the graph with nodes V and edges H = (E - F). The
// resulting graph has the same set of nodes as the original graph though some
// of these nodes may have no incoming or outgoing edges.
//
// Returns a labeled graph obtained by deleting 'edges' from the input graph.
std::unique_ptr<LabeledGraph> DeleteEdges(const LabeledGraph& graph,
                                          const set<EdgeId>& edges);

// The Quotient graph Q of a graph G is a graph whose nodes are blocks of a
// partition of the vertices of G and where block B is adjacent to block C if
// some node in B is adjacent to some node in C with respect to the edge set of
// G. In other words, if G has edge set E and node set V and R is the
// equivalence relation induced by the partition, then the quotient graph has
// node set V/R and edge set {([u]R, [v]R) | (u, v) ∈ E(G)}. For example, the
// condensation of a strongly connected graph is the quotient graph where the
// strongly connected components form the blocks of the partition.
//
// Source: https://en.wikipedia.org/wiki/Quotient_graph
//
// Returns a labeled graph that is the quotient of the input graph with respect
// to the partition.
//
// The 'partition' maps each node in the graph onto some block of the partition,
// which we represent by an integer label.
//
// Requires that:
// - The 'partition' has every node in 'graph' as a key.
// - 'config' meets the requirements of a QuotientConfig, given above.
std::unique_ptr<LabeledGraph> QuotientGraph(
    const LabeledGraph& input_graph, const std::map<NodeId, int>& partition,
    const QuotientConfig& config);

// Edge contraction replaces an edge (u, v) with a new node w such that for each
// edge (x, u) or (x, v) in the input graph there is an edge (x, w) in the
// output graph. This applies likewise for edges (u, x) and (u, v).
std::unique_ptr<LabeledGraph> ContractEdges(const LabeledGraph& graph,
                                            const set<EdgeId>& edges,
                                            const QuotientConfig& config);

// Folding node v removes v from the graph, and replacing it with a complete
// bipartite graph between its predecessors and successors. This means that for
// each (u, v) and (v, w) in the original graph, there is an edge (u, w) in the
// output graph.
std::unique_ptr<LabeledGraph> FoldNodes(const LabeledGraph& graph,
                                        const FoldLabelFn& fold_label_fn,
                                        const set<NodeId>& nodes);
}  // namespace graph

}  // namespace tervuren

#endif  // THIRD_PARTY_LOGLE_GRAPH_TRANSFORMER_H_
