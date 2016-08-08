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

#include "curio_analyzer.h"

#include "gtest.h"
#include "util/status.h"

namespace tervuren {
namespace {

// Returns a JSON object constructed from 'input'.
std::unique_ptr<::Json::Value> CreateJSON(const string& input) {
  std::unique_ptr<::Json::Value> doc(new ::Json::Value);
  ::Json::Reader reader;
  reader.parse(input, *doc, false /*Ignore comments.*/);
  return doc;
}

// Creates a JSON object from 'input' and returns the status object obtained
// when the CurioAnalyzer is initialized.
util::Status GetInitializationStatus(const string& input) {
  std::unique_ptr<::Json::Value> doc = CreateJSON(input);
  CurioAnalyzer analyzer;
  return analyzer.Initialize(std::move(doc));
}

// Reject an empty JSON object from the input.
TEST(CurioAnalyzerTest, RequiresNonNullJSONDoc) {
  std::unique_ptr<::Json::Value> doc;
  CurioAnalyzer curio_analyzer;
  EXPECT_FALSE(curio_analyzer.Initialize(std::move(doc)).ok());
}

// Test that the CurioAnalyzer can only be initialized with a non-empty JSON
// object.
TEST(CurioAnalyzerTest, TestInitialization) {
  // Reject the empty string.
  EXPECT_FALSE(GetInitializationStatus("").ok());
  // Reject the empty object.
  EXPECT_FALSE(GetInitializationStatus("{}").ok());
  // Reject the empty array.
  EXPECT_FALSE(GetInitializationStatus("[]").ok());
  // Reject a string.
  EXPECT_FALSE(GetInitializationStatus("abc").ok());
  // Reject the non-empty array.
  EXPECT_FALSE(GetInitializationStatus(R"(["a", "b"])").ok());
  // Accept a non-empty object.
  EXPECT_TRUE(GetInitializationStatus(R"({"name" : {}})").ok());
}

// Test that the StreamDependencyGraph is empty after initialization.
TEST(CurioAnalyzerTest, InitializationDoesNotProcess) {
  std::unique_ptr<::Json::Value> doc = CreateJSON(R"({"name" : {}})");
  CurioAnalyzer curio_analyzer;
  EXPECT_TRUE(curio_analyzer.Initialize(std::move(doc)).ok());
  EXPECT_EQ(0, curio_analyzer.NumStreamsSkipped());
  EXPECT_EQ(0, curio_analyzer.NumStreamsProcessed());
}

static const vector<std::pair<string, std::pair<int, int>>> malformed_data = {
    // One stream missing all information.
    {R"({"[/path/to/stream:stream1]" : {}})", {0, 1}},
    // Two streams missing all information.
    {R"({"[/path/to/stream:stream2]" : {},
             "[/path/to/stream:stream3]" : {}})",
     {0, 2}},
    // A stream missing ID and children definitions.
    {R"({"[/path/to/stream:stream4]" : {"Node":{}}})", {0, 1}},
    // A stream with empty ID and no children definition.
    {R"({"[/path/to/stream:stream5]" : {"Node":{"ID":{}}}})", {0, 1}},
    // A stream with no children definition.
    {R"({"[/path/to/stream:stream6]" :
      {"Node":{"ID":{"Package":"/path/to/stream", "Name":"stream6"}}}})",
     {0, 1}},
};

static const vector<std::pair<string, std::pair<int, int>>> streams = {
    // A stream with an empty set of children is valid input.
    {R"({"[/path/to/stream:stream1]" :
      {"Node":{"ID":{"Package":"/path/to/stream", "Name":"stream1"}},
       "Children": {}}})",
     {1, 0}},
    {R"({"[/path/to/stream:stream2]" :
      {"Node":{"ID":{"Package":"/path/to/stream2", "Name":"stream2"}},
       "Children": {}},
      "[/path/to/stream:stream3]" :
      {"Node":{"ID":{"Package":"/path/to/stream3", "Name":"stream3"}},
        "Children": {}}})",
     {2, 0}},
};

TEST(CurioAnalyzerTest, TestMalformedObjectDetection) {
  std::unique_ptr<::Json::Value> doc;
  for (const auto& stream_data : malformed_data) {
    doc = CreateJSON(stream_data.first);
    CurioAnalyzer curio_analyzer;
    EXPECT_TRUE(curio_analyzer.Initialize(std::move(doc)).ok())
        << "Error initializing the doc " << stream_data.first;
    EXPECT_TRUE(curio_analyzer.BuildDependencyGraph().ok())
        << "Error building dependency graph for " << stream_data.first;
    EXPECT_EQ(stream_data.second.first, curio_analyzer.NumStreamsProcessed())
        << " Error when processing " << stream_data.first;
    EXPECT_EQ(stream_data.second.second, curio_analyzer.NumStreamsSkipped())
        << " Error when processing " << stream_data.first;
  }
}

TEST(CurioAnalyzerTest, TestStreamProcessing) {
  std::unique_ptr<::Json::Value> doc;
  for (const auto& stream_data : streams) {
    doc = CreateJSON(stream_data.first);
    CurioAnalyzer curio_analyzer;
    EXPECT_TRUE(curio_analyzer.Initialize(std::move(doc)).ok())
        << "Error initializing the doc " << stream_data.first;
    EXPECT_TRUE(curio_analyzer.BuildDependencyGraph().ok())
        << "Error building dependency graph for " << stream_data.first;
    EXPECT_EQ(stream_data.second.first, curio_analyzer.NumStreamsProcessed())
        << " Error when processing " << stream_data.first;
    EXPECT_EQ(stream_data.second.second, curio_analyzer.NumStreamsSkipped())
        << " Error when processing " << stream_data.first;
  }
}

// Test that if there are signals with no dependencies, then no nodes are added
// to the graph.
TEST(CurioAnalyzerTest, ConstructsEmptyGraphs) {
  string stream = R"({"[/path/to/stream:stream1]" :
      {"Node":{"ID":{"Package":"/path/to/stream1", "Name":"stream1"}},
       "Children": {}}})";
  std::unique_ptr<::Json::Value> doc;
  doc = CreateJSON(stream);
  CurioAnalyzer curio_analyzer;
  ASSERT_TRUE(curio_analyzer.Initialize(std::move(doc)).ok());
  EXPECT_TRUE(curio_analyzer.BuildDependencyGraph().ok());
  EXPECT_EQ(1, curio_analyzer.NumStreamsProcessed());
  EXPECT_EQ(0, curio_analyzer.NumGraphNodes());
  EXPECT_EQ(0, curio_analyzer.NumGraphEdges());
  stream.pop_back();
  stream += R"(,"[/path/to/stream:stream2]" :
      {"Node":{"ID":{"Package":"/path/to/stream2", "Name":"stream2"}},
        "Children": {}})";
  stream += "}";
  doc = CreateJSON(stream);
  ASSERT_TRUE(curio_analyzer.Initialize(std::move(doc)).ok());
  EXPECT_TRUE(curio_analyzer.BuildDependencyGraph().ok());
  EXPECT_EQ(2, curio_analyzer.NumStreamsProcessed());
  EXPECT_EQ(0, curio_analyzer.NumStreamsSkipped());
  EXPECT_EQ(0, curio_analyzer.NumGraphNodes());
  EXPECT_EQ(0, curio_analyzer.NumGraphEdges());
}

// Create the dependency graph below.
//  stream1 -> stream2 -> stream3
TEST(CurioAnalyzerTest, ChainGraphs) {
  string stream = R"({"[/path/to/stream:stream1]" :
      {"Node":{"ID":{"Package":"/path/to/stream1", "Name":"stream1"}},
       "Children": {"[/path/to/stream:stream2]" :
      {"Node":{"ID":{"Package":"/path/to/stream2", "Name":"stream2"}},
        "Children": {"[/path/to/stream:stream3]" :
      {"Node":{"ID":{"Package":"/path/to/stream3", "Name":"stream3"}},
        "Children": {}}}}}}})";
  std::unique_ptr<::Json::Value> doc;
  doc = CreateJSON(stream);
  CurioAnalyzer curio_analyzer;
  ASSERT_TRUE(curio_analyzer.Initialize(std::move(doc)).ok());
  EXPECT_TRUE(curio_analyzer.BuildDependencyGraph().ok());
  EXPECT_EQ(3, curio_analyzer.NumStreamsProcessed());
  EXPECT_EQ(0, curio_analyzer.NumStreamsSkipped());
  EXPECT_EQ(3, curio_analyzer.NumGraphNodes());
  EXPECT_EQ(2, curio_analyzer.NumGraphEdges());
  // Create the dependency graph consisting of two unconnected graphs:
  //  stream1 -> stream2 -> stream3
  //  stream4 -> stream5
  stream.pop_back();
  stream += R"(, "[/path/to/stream:stream4]" :
      {"Node":{"ID":{"Package":"/path/to/stream4", "Name":"stream4"}},
        "Children": {"[/path/to/stream:stream4]" :
      {"Node":{"ID":{"Package":"/path/to/stream5", "Name":"stream5"}},
        "Children": {}}}})";
  stream += "}";
  doc = CreateJSON(stream);
  ASSERT_TRUE(curio_analyzer.Initialize(std::move(doc)).ok());
  EXPECT_TRUE(curio_analyzer.BuildDependencyGraph().ok());
  EXPECT_EQ(5, curio_analyzer.NumStreamsProcessed());
  EXPECT_EQ(0, curio_analyzer.NumStreamsSkipped());
  EXPECT_EQ(5, curio_analyzer.NumGraphNodes());
  EXPECT_EQ(3, curio_analyzer.NumGraphEdges());
}

// Create the star-shaped dependency graph:
//             stream4
//                ^
//                |
//  stream2 <- stream1 -> stream3
TEST(CurioAnalyzerTest, StarGraphs) {
  string stream = R"({"[/path/to/stream:stream1]" :
      {"Node":{"ID":{"Package":"/path/to/stream1", "Name":"stream1"}},
       "Children": {"[/path/to/stream:stream2]" :
      {"Node":{"ID":{"Package":"/path/to/stream2", "Name":"stream2"}},
        "Children": {}},
      "[/path/to/stream:stream3]" :
      {"Node":{"ID":{"Package":"/path/to/stream3", "Name":"stream3"}},
        "Children": {}},
      "[/path/to/stream:stream4]" :
      {"Node":{"ID":{"Package":"/path/to/stream4", "Name":"stream4"}},
        "Children": {}}
       }}})";
  std::unique_ptr<::Json::Value> doc;
  doc = CreateJSON(stream);
  CurioAnalyzer curio_analyzer;
  ASSERT_TRUE(curio_analyzer.Initialize(std::move(doc)).ok());
  EXPECT_TRUE(curio_analyzer.BuildDependencyGraph().ok());
  EXPECT_EQ(4, curio_analyzer.NumStreamsProcessed());
  EXPECT_EQ(0, curio_analyzer.NumStreamsSkipped());
  EXPECT_EQ(4, curio_analyzer.NumGraphNodes());
  EXPECT_EQ(3, curio_analyzer.NumGraphEdges());
}

// Create the directed, acyclic dependency graph below.
//   stream2 -->--+
//      ^         |
//      |         |
//   stream1 -> stream3
TEST(CurioAnalyzerTest, SimpleDAG) {
  string stream = R"({"[/path/to/stream:stream1]" :
      {"Node":{"ID":{"Package":"/path/to/stream1", "Name":"stream1"}},
       "Children": {"[/path/to/stream:stream2]" :
      {"Node":{"ID":{"Package":"/path/to/stream2", "Name":"stream2"}},
        "Children": {"[/path/to/stream:stream3]" :
      {"Node":{"ID":{"Package":"/path/to/stream3", "Name":"stream3"}},
        "Children": {}}}},
      "[/path/to/stream:stream3]" :
      {"Node":{"ID":{"Package":"/path/to/stream3", "Name":"stream3"}},
        "Children": {}}
       }}})";
  std::unique_ptr<::Json::Value> doc;
  doc = CreateJSON(stream);
  CurioAnalyzer curio_analyzer;
  ASSERT_TRUE(curio_analyzer.Initialize(std::move(doc)).ok());
  EXPECT_TRUE(curio_analyzer.BuildDependencyGraph().ok());
  EXPECT_EQ(4, curio_analyzer.NumStreamsProcessed());
  EXPECT_EQ(0, curio_analyzer.NumStreamsSkipped());
  EXPECT_EQ(3, curio_analyzer.NumGraphNodes());
  EXPECT_EQ(3, curio_analyzer.NumGraphEdges());
}

// Construct the self-dependent graphs below.
//  stream1 --> stream1
TEST(CurioAnalyzerTest, SimpleLoopGraphs) {
  string stream = R"({"[/path/to/stream:stream1]" :
      {"Node":{"ID":{"Package":"/path/to/stream1", "Name":"stream1"}},
       "Children": {"[/path/to/stream:stream1]" :
      {"Node":{"ID":{"Package":"/path/to/stream1", "Name":"stream1"}},
       "Children": {}}}}})";
  std::unique_ptr<::Json::Value> doc;
  doc = CreateJSON(stream);
  CurioAnalyzer curio_analyzer;
  ASSERT_TRUE(curio_analyzer.Initialize(std::move(doc)).ok());
  EXPECT_TRUE(curio_analyzer.BuildDependencyGraph().ok());
  EXPECT_EQ(2, curio_analyzer.NumStreamsProcessed());
  EXPECT_EQ(1, curio_analyzer.NumGraphNodes());
  EXPECT_EQ(1, curio_analyzer.NumGraphEdges());
  // Construct graphs with simple loops.
  //  stream1 --> stream1
  //  stream2 <-> stream3
  stream.pop_back();
  stream += R"(, "[/path/to/stream:stream2]" :
      {"Node":{"ID":{"Package":"/path/to/stream2", "Name":"stream2"}},
        "Children": {"[/path/to/stream:stream2]" :
      {"Node":{"ID":{"Package":"/path/to/stream3", "Name":"stream3"}},
        "Children": {"[/path/to/stream:stream2]" :
      {"Node":{"ID":{"Package":"/path/to/stream2", "Name":"stream2"}},
        "Children": {}}}}}})";
  stream += "}";
  doc = CreateJSON(stream);
  ASSERT_TRUE(curio_analyzer.Initialize(std::move(doc)).ok());
  EXPECT_TRUE(curio_analyzer.BuildDependencyGraph().ok());
  EXPECT_EQ(5, curio_analyzer.NumStreamsProcessed());
  EXPECT_EQ(0, curio_analyzer.NumStreamsSkipped());
  EXPECT_EQ(3, curio_analyzer.NumGraphNodes());
  EXPECT_EQ(3, curio_analyzer.NumGraphEdges());
}

// The JSON input defines the stream below, but stream3 is malformed.
//  stream1 -> stream2 -> stream3
// The expected graph is:
//  stream1 -> stream2
TEST(CurioAnalyzerTest, MalformedChainEnd) {
  string stream = R"({"[/path/to/stream:stream1]" :
      {"Node":{"ID":{"Package":"/path/to/stream1", "Name":"stream1"}},
       "Children": {"[/path/to/stream:stream2]" :
      {"Node":{"ID":{"Package":"/path/to/stream2", "Name":"stream2"}},
        "Children": {"[/path/to/stream:stream3]" :
      {"Node":{"ID":{"Package":"/path/to/stream3", "Name":"stream3"}}}}}}}})";
  std::unique_ptr<::Json::Value> doc;
  doc = CreateJSON(stream);
  CurioAnalyzer curio_analyzer;
  ASSERT_TRUE(curio_analyzer.Initialize(std::move(doc)).ok());
  EXPECT_TRUE(curio_analyzer.BuildDependencyGraph().ok());
  EXPECT_EQ(2, curio_analyzer.NumStreamsProcessed());
  EXPECT_EQ(1, curio_analyzer.NumStreamsSkipped());
  EXPECT_EQ(2, curio_analyzer.NumGraphNodes());
  EXPECT_EQ(1, curio_analyzer.NumGraphEdges());
}

// The JSON input defines the stream below, but stream2 is malformed because it
// does not have the "Package" and "Name" fields.
//  stream1 -> stream2 -> stream3
// The expected graph is:
//  stream1
// because the subtree below stream2 is not processed. Note also that only 1
// streams is processed and 1 is skipped because stream 3 is never read.
TEST(CurioAnalyzerTest, MalformedChainMid) {
  string stream = R"({"[/path/to/stream:stream1]" :
      {"Node":{"ID":{"Package":"/path/to/stream1", "Name":"stream1"}},
       "Children": {"[/path/to/stream:stream2]" :
      {"Node":{"ID":{}},
        "Children": {"[/path/to/stream:stream3]" :
      {"Node":{"ID":{"Package":"/path/to/stream3", "Name":"stream3"}},
        "Children": {}}}}}}})";
  std::unique_ptr<::Json::Value> doc;
  doc = CreateJSON(stream);
  CurioAnalyzer curio_analyzer;
  ASSERT_TRUE(curio_analyzer.Initialize(std::move(doc)).ok());
  EXPECT_TRUE(curio_analyzer.BuildDependencyGraph().ok());
  EXPECT_EQ(1, curio_analyzer.NumStreamsProcessed());
  EXPECT_EQ(1, curio_analyzer.NumStreamsSkipped());
  EXPECT_EQ(0, curio_analyzer.NumGraphNodes());
  EXPECT_EQ(0, curio_analyzer.NumGraphEdges());
}

}  // namespace
}  // namespace tervuren
