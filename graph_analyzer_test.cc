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
#include "graph_analyzer.h"

#include "gtest.h"
#include "test_graphs.h"

namespace morphie {
namespace {

std::map<NodeId, int> SingleBlockPartition(const LabeledGraph& graph) {
  std::map<NodeId, int> partition;
  auto end_it = graph.NodeSetEnd();
  for (auto node_it = graph.NodeSetBegin(); node_it != end_it; ++node_it) {
    partition.insert({*node_it, 0});
  }
  return partition;
}

std::map<NodeId, int> IdentityPartition(const LabeledGraph& graph) {
  std::map<NodeId, int> partition;
  auto end_it = graph.NodeSetEnd();
  int i = 0;
  for (auto node_it = graph.NodeSetBegin(); node_it != end_it; ++node_it, ++i) {
    partition.insert({*node_it, i});
  }
  return partition;
}

// Tests a single block partition on a simple graph.
// input_graph:
// 0 -> 1
TEST(GraphAnalyzerTest, SimpleSingleBlockSplit) {
  test::WeightedGraph path;
  test::GetPathGraph(2, &path);
  ASSERT_EQ(2, path.NumNodes());
  ASSERT_EQ(1, path.NumEdges());
  const LabeledGraph& input_graph = *path.GetGraph();

  auto partition = SingleBlockPartition(input_graph);
  auto refinement = graph_analyzer::RefinePartition(input_graph, partition);
  auto ref_it = refinement.begin();
  int id1 = ref_it->second;
  ++ref_it;
  int id2 = ref_it->second;
  EXPECT_NE(id1, id2);
}

// Tests a single block partition on a simple graph.
// input_graph:
// 0 -> 1 -> 2 -> 3 -> 4 -> 5
TEST(GraphAnalyzerTest, LongSingleBlockSplit) {
  test::WeightedGraph path;
  test::GetPathGraph(6, &path);
  ASSERT_EQ(6, path.NumNodes());
  ASSERT_EQ(5, path.NumEdges());
  const LabeledGraph& input_graph = *path.GetGraph();

  auto partition = SingleBlockPartition(input_graph);
  auto refinement = graph_analyzer::RefinePartition(input_graph, partition);
  auto end_it = refinement.end();
  std::set<int> ids;
  for (auto ref_it = refinement.begin(); ref_it != end_it; ++ref_it) {
    ids.insert(ref_it->second);
  }
  EXPECT_EQ(6, ids.size());
}

// Tests that the coarsest stable refinement of the identity partition is just
// the identity.
// input_graph:
// 0 -> 1 -> 2
// ^         |
// |         v
// 5 <- 4 <- 3
TEST(GraphAnalyzerTest, IdentityRefinement) {
  test::WeightedGraph cycle;
  test::GetCycleGraph(6, &cycle);
  ASSERT_EQ(6, cycle.NumNodes());
  ASSERT_EQ(6, cycle.NumEdges());
  const LabeledGraph& input_graph = *cycle.GetGraph();

  auto partition = IdentityPartition(input_graph);
  auto refinement = graph_analyzer::RefinePartition(input_graph, partition);
  auto end_it = refinement.end();
  std::set<int> ids;
  for (auto ref_it = refinement.begin(); ref_it != end_it; ++ref_it) {
    ids.insert(ref_it->second);
  }
  EXPECT_EQ(6, ids.size());
}

// Tests graph_analyzer::RefinePartition on the following partition and graph.
// partition:
// {0, 1, 4, 5}, {2, 3, 6, 7}
// input_graph:
// 0 -> 1 -> 2
// ^         |
// |         v
// 7         3
// ^         |
// |         v
// 6 <- 5 <- 4
//
// Expected output:
// {0, 4}, {1, 5}, {2, 6}, {3, 7}
TEST(GraphAnalyzerTest, CycleRefinement) {
  test::WeightedGraph cycle;
  test::GetCycleGraph(8, &cycle);
  ASSERT_EQ(8, cycle.NumNodes());
  ASSERT_EQ(8, cycle.NumEdges());
  const LabeledGraph& input_graph = *cycle.GetGraph();

  std::map<NodeId, int> ids;
  std::map<NodeId, int> partition;
  NodeId current_node = *input_graph.NodeSetBegin();
  for (int index = 0; index < 8; ++index) {
    ids.insert({current_node, index});
    int partition_id = index % 4 < 2 ? 0 : 1;
    partition.insert({current_node, partition_id});
    current_node = *(input_graph.GetSuccessors(current_node).begin());
  }
  auto node_end_it = input_graph.NodeSetEnd();
  auto refinement = graph_analyzer::RefinePartition(input_graph, partition);
  for (auto node_it_1 = input_graph.NodeSetBegin(); node_it_1 != node_end_it;
       ++node_it_1) {
    for (auto node_it_2 = std::next(node_it_1); node_it_2 != node_end_it;
         ++node_it_2) {
      NodeId node_1 = *node_it_1;
      NodeId node_2 = *node_it_2;
      if (ids[node_1] % 4 == ids[node_2] % 4) {
        EXPECT_EQ(refinement[node_1], refinement[node_2]);
      } else {
        EXPECT_NE(refinement[node_1], refinement[node_2]);
      }
    }
  }
}

}  // namespace
}  // namespace morphie
