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

#include "third_party/logle/test_graphs.h"

#include "third_party/logle/gtest.h"

namespace third_party_logle {
namespace test {
namespace {

// Test that path-shaped graphs labeled with increasing numbers are created.
TEST(TestGraphsTest, CreatesPathGraphs) {
  // Create the graph with one node and no edges.
  WeightedGraph one_path;
  test::GetPathGraph(1, &one_path);
  EXPECT_EQ(1, one_path.NumNodes());
  EXPECT_EQ(0, one_path.NumEdges());
  // Create the graph {0 -0-> 1, ... , 8 -8-> 9 } with 10 nodes and 8 edges.
  WeightedGraph ten_path;
  test::GetPathGraph(10, &ten_path);
  EXPECT_EQ(10, ten_path.NumNodes());
  EXPECT_EQ(9, ten_path.NumEdges());
}

// Test that cycle-shaped graphs labeled with increasing numbers are created.
TEST(TestGraphsTest, CreatesCycleGraphs) {
  // Create the graph {0 -0-> 0 } with one node and one edge.
  WeightedGraph one_cycle;
  test::GetCycleGraph(1, &one_cycle);
  EXPECT_EQ(1, one_cycle.NumNodes());
  EXPECT_EQ(1, one_cycle.NumEdges());
  // Create the graph {0 -0-> 1, ... , 9 -9-> 0 } with 10 nodes and 10 edges.
  WeightedGraph ten_cycle;
  test::GetCycleGraph(10, &ten_cycle);
  EXPECT_EQ(10, ten_cycle.NumNodes());
  EXPECT_EQ(10, ten_cycle.NumEdges());
}

}  // namespace
}  // namespace test
}  // namespace third_party_logle
