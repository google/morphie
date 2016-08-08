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

#include "plaso_event_graph.h"

#include <memory>  // for __alloc_traits<>::value_type

#include "gtest.h"
#include "plaso_event.h"
#include "plaso_event.pb.h"
#include "util/status.h"
#include "util/time_utils.h"
#include "value.h"

namespace tervuren {
namespace {

namespace value = ast::value;

TEST(PlasoEventGraphDeathTest, UninitializedAPICalls) {
  PlasoEventGraph graph;
  EXPECT_DEATH({ graph.NumNodes(); }, ".*");
  EXPECT_DEATH({ graph.NumEdges(); }, ".*");
  PlasoEvent event_data;
  EXPECT_DEATH({ graph.ProcessEvent(event_data); }, ".*");
  EXPECT_DEATH({ graph.AddTemporalEdges(); }, ".*");
}

class PlasoEventGraphTest : public ::testing::Test {
 protected:
  void SetUp() override {
    util::Status s = graph_.Initialize();
    EXPECT_TRUE(s.ok());
  }

  PlasoEventGraph graph_;
};

// Returns a PlasoEvent proto with the timestamp and description fields set.
PlasoEvent GetProto() {
  PlasoEvent event;
  int64_t unix_micros;
  if (util::RFC3339ToUnixMicros("2012-04-03T00:25:22+00:00", &unix_micros)) {
    event.set_timestamp(unix_micros);
  }
  event.set_desc("ctime");
  return event;
}

// Test processing of an event with only a timestamp and description. Adding the
// same event multiple times causes multiple nodes to be added to the graph.
TEST_F(PlasoEventGraphTest, ProcessResourceslessEvents) {
  EXPECT_EQ(0, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
  PlasoEvent event = GetProto();
  graph_.ProcessEvent(event);
  EXPECT_EQ(1, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
  graph_.ProcessEvent(event);
  EXPECT_EQ(2, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
  graph_.ProcessEvent(event);
  EXPECT_EQ(3, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
}

// There are no temporal edges between event with the same timestamp.
TEST_F(PlasoEventGraphTest, NoEdgesBetweenConcurrentEvents) {
  PlasoEvent event = GetProto();
  graph_.ProcessEvent(event);
  graph_.ProcessEvent(event);
  graph_.ProcessEvent(event);
  graph_.AddTemporalEdges();
  EXPECT_EQ(3, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
}

// There should be edges between temporally ordered, subsequent events.
TEST_F(PlasoEventGraphTest, TemporalEdgesBetweenEvents) {
  PlasoEvent event = GetProto();
  graph_.ProcessEvent(event);
  event.set_timestamp(event.timestamp() + (int64_t)5000000);
  graph_.ProcessEvent(event);
  event.set_timestamp(event.timestamp() + (int64_t)5000000);
  graph_.ProcessEvent(event);
  graph_.AddTemporalEdges();
  EXPECT_EQ(3, graph_.NumNodes());
  EXPECT_EQ(2, graph_.NumEdges());
}

TEST_F(PlasoEventGraphTest, ProcessEventsWithFiles) {
  PlasoEvent event = GetProto();
  File file = plaso::ParseFilename("example.txt");
  *event.mutable_source_file() = file;
  *event.mutable_target_file() = file;
  graph_.ProcessEvent(event);
  // There should be a node for the event, one for the file, and two edges
  // between them as the file is a source and a target.
  EXPECT_EQ(2, graph_.NumNodes());
  EXPECT_EQ(2, graph_.NumEdges());
  // Adding the same event information twice should result in two events but
  // still only one file. Since there is no target, there should be three edges.
  event.clear_source_file();
  graph_.ProcessEvent(event);
  EXPECT_EQ(3, graph_.NumNodes());
  EXPECT_EQ(3, graph_.NumEdges());
}

TEST_F(PlasoEventGraphTest, ProcessEventsWithURLs) {
  PlasoEvent event = GetProto();
  event.set_source_url("www.google.com");
  graph_.ProcessEvent(event);
  // There should be a node for the event, one for the URL, and an edge between
  // them.
  EXPECT_EQ(2, graph_.NumNodes());
  EXPECT_EQ(1, graph_.NumEdges());
  // Adding the same event information twice should result in two events but
  // still only one URL.
  graph_.ProcessEvent(event);
  EXPECT_EQ(3, graph_.NumNodes());
  EXPECT_EQ(2, graph_.NumEdges());
}

}  // namespace
}  // namespace tervuren
