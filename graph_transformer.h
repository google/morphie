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
// The 'partition' maps each node in the graph onto some block of the partition,
// which we represent by an integer label.
//
// Returns the labeled graph that results from the quotient operation on the
// input graph with respect to the input partition.
// - Requires that 'partition' has every node in 'graph' as a key.
std::unique_ptr<LabeledGraph> QuotientGraph(
                              const LabeledGraph& graph,
                              const std::map<NodeId, int>& partition);

}  // namespace graph

}  // namespace third_party_logle

#endif  // THIRD_PARTY_LOGLE_GRAPH_TRANSFORMER_H_
