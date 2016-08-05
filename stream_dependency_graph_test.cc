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

#include "stream_dependency_graph.h"

#include <vector>

#include "base/string.h"
#include "gtest.h"

namespace tervuren {
namespace {

TEST(PlasoEventGraphDeathTest, UninitializedCalls) {
  StreamDependencyGraph graph;
  const char kInitializationRegEx[] = ".*not initialized.*";
  EXPECT_DEATH({ graph.NumNodes(); }, kInitializationRegEx);
  EXPECT_DEATH({ graph.NumEdges(); }, kInitializationRegEx);
  EXPECT_DEATH({ graph.AddDependency("", "", "", ""); }, kInitializationRegEx);
  EXPECT_DEATH({ graph.ToDot(); }, kInitializationRegEx);
}

static const vector<std::pair<string, string>> streams = {
    {"[/path/to/stream:stream1]", "stream1"},
    {"[/path/to/stream:stream2]", "stream2"},
    {"[/path/to/stream:stream3]", "stream3"},
};

// Construct the dependency graph below and check that the number of nodes and
// edges is as expected at each stage of the construction.
//
//         stream3
//         ^     ^
//         |     |
//   stream1 --> stream2
TEST(PlasoEventGraphTest, AddDependency) {
  StreamDependencyGraph graph;
  EXPECT_TRUE(graph.Initialize().ok());
  graph.AddDependency(streams[0].first, streams[0].second, streams[1].first,
                      streams[1].second);
  EXPECT_EQ(2, graph.NumNodes());
  EXPECT_EQ(1, graph.NumEdges());
  graph.AddDependency(streams[0].first, streams[0].second, streams[2].first,
                      streams[2].second);
  EXPECT_EQ(3, graph.NumNodes());
  EXPECT_EQ(2, graph.NumEdges());
  graph.AddDependency(streams[1].first, streams[1].second, streams[2].first,
                      streams[2].second);
  EXPECT_EQ(3, graph.NumNodes());
  EXPECT_EQ(3, graph.NumEdges());
  // Test that there can be at most one dependency edge between two nodes.
  graph.AddDependency(streams[1].first, streams[1].second, streams[2].first,
                      streams[2].second);
  EXPECT_EQ(3, graph.NumNodes());
  EXPECT_EQ(3, graph.NumEdges());
}

}  // namespace
}  // namespace tervuren
