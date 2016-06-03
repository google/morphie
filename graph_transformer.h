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
#ifndef LOGLE_GRAPH_TRANSFORMER_H_
#define LOGLE_GRAPH_TRANSFORMER_H_

#include <memory>
#include <set>

#include "labeled_graph.h"

namespace logle {
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
                                          const std::set<NodeId>& nodes);

}  // namespace graph

}  // logle

#endif  // LOGLE_GRAPH_TRANSFORMER_H_
