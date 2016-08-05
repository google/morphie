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

// This file contains the definitions of different types of labeled graphs for
// use in testing graph algorithms.
#ifndef THIRD_PARTY_LOGLE_TEST_GRAPHS_H_
#define THIRD_PARTY_LOGLE_TEST_GRAPHS_H_

#include <set>

#include "third_party/logle/base/string.h"
#include "third_party/logle/labeled_graph.h"
#include "third_party/logle/util/status.h"

namespace tervuren {
namespace test {

// A weighted graph is one in which the nodes and edges have int weights. There
// can be multiple nodes with the same weight and multiple edges of the same
// weight between a given pair of nodes. A WeightedGraph must be initialized by
// calling WeightedGraph::Initialize() before it can be used.
class WeightedGraph {
 public:
  WeightedGraph() : is_initialized_(false) {}

  // Initializes the graph and returns one of the following error codes:
  // - Status::OK - if a graph with the appropriate types was created.
  // - Status::INTERNAL - otherwise, with the reason accessible via the
  //   Status::error_message() function of the Status object.
  util::Status Initialize();

  // Adds a new node with the given weight to the graph and returns the
  // identifier for that node.
  NodeId AddNode(int node_weight);

  // Adds a new, directed edge with the given weight to the graph and returns
  // the identifier for that edge.
  EdgeId AddEdge(NodeId src, NodeId tgt, int edge_weight);

  // Returns the set of identifiers of the nodes with a given weight.
  set<NodeId> GetNodes(int node_weight) const;

  const LabeledGraph* GetGraph() const;

  int NumNodes() const;
  int NumEdges() const;

  // Returns a representation of the graph in Graphviz DOT format.
  string ToDot() const;

 private:
  bool is_initialized_;
  LabeledGraph graph_;
};  // class IntGraph

// A path graph with k+1 nodes, for k greater than or equal to 0, has
//  - a node with weight j for every j in [0,k], and
//  - an edge j -> j+1 with weight j for j in [0, k-1].
// A path graph can be drawn as a straight line.
void GetPathGraph(int num_nodes, WeightedGraph* graph);

// A cycle graph with k+1 nodes, for k greater than or equal to 0, has
//  - a node with weight j for every j in [0,k], and
//  - an edge j -> (j+1 mod k) with weight j for j in [0, k].
// A cycle graph can be drawn as a simple cycle.
void GetCycleGraph(int num_nodes, WeightedGraph* graph);

// Returns true if 'graph' is a path graph.
bool IsPath(const LabeledGraph& graph);

// Returns true if 'graph' is a cycle graph.
bool IsCycle(const LabeledGraph& graph);

}  // namespace test
}  // namespace tervuren
#endif  // THIRD_PARTY_LOGLE_TEST_GRAPHS_H_
