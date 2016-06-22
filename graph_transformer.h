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

#include "third_party/logle/labeled_graph.h"

namespace third_party_logle {
// The 'graph' namespace contains functions for manipulating graphs.
namespace graph {

// A NodeLabelFn takes in a set of nodes in a graph and generates a new label.
using NodeLabelFn =
    std::function<TaggedAST(const LabeledGraph&, const std::set<NodeId>&)>;

// A EdgeLabelFn takes as input a set of edges in a graph and generates a new
// label.
using EdgeLabelFn =
    std::function<TaggedAST(const LabeledGraph&, const std::set<EdgeId>&)>;

// If G = (V, E) is a graph and N is a subset of nodes of V, the result of
// deleting N from G is the graph with vertices W = (V - N) and with those edges
// in E whose source and target are both in W.
//
// Returns a labeled graph obtained by deleting 'nodes' from the input graph.
// Has time complexity linear in the number of edges of the input graph and
// space complexity linear in the number of nodes and edges of the ouput graph.
// - Requires that 'nodes' contains valid node identifiers of the input graph.
std::unique_ptr<LabeledGraph> DeleteNodes(const LabeledGraph& graph,
                                          const set<NodeId>& nodes);

// The Quotient graph Q of a graph G is a graph whose vertices are blocks of a
// partition of the vertices of G and where block B is adjacent to block C if
// some vertex in B is adjacent to some vertex in C with respect to the edge
// set of G. In other words, if G has edge set E and vertex set V and R is
// the equivalence relation induced by the partition, then the quotient graph
// has vertex set V/R and edge set {([u]R, [v]R) | (u, v) ∈ E(G)}. For example,
// the condensation of a strongly connected graph is the quotient graph where
// the strongly connected components form the blocks of the partition.
//
// Source: https://en.wikipedia.org/wiki/Quotient_graph
//
// Returns a labeled graph that is the quotient of the input graph with respect
// to the partition.
//
// The 'output_graph_type' specifies the types of nodes and edges permitted in
// the output graph.
// The 'partition' maps each node in the graph onto some block of the partition,
// which we represent by an integer label.
// The 'node_label_fn' determines how the blocks are labeled in the output.
// The 'edge_label_fn' determines how the edges between the blocks are labeled.
// This function is only applied if 'allow_multi_edges' is false.
// The flag 'allow_multi_edges' dictates whether the output graph will allow
// multi-edges between nodes, or will instead have a single edge, labeled by
// 'edge_label_fn'.
//
// Requires that:
// - The 'partition' has every node in 'graph' as a key.
// - Both 'node_label_fn' and 'edge_label_fn' respect the types of
//   'output_graph_type'.
// - If 'allow_multi_edges' is true, the edge types of 'input_graph' and
//   'output_graph_type' must be the same.
std::unique_ptr<LabeledGraph> QuotientGraph(
    const LabeledGraph& input_graph, const LabeledGraph& output_graph_type,
    const std::map<NodeId, int>& partition, const NodeLabelFn& node_label_fn,
    const EdgeLabelFn& edge_label_fn, bool allow_multi_edges);
}  // namespace graph

}  // namespace third_party_logle

#endif  // THIRD_PARTY_LOGLE_GRAPH_TRANSFORMER_H_
