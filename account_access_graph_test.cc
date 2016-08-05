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

#include "third_party/logle/account_access_graph.h"

#include "third_party/logle/account_access_defs.h"
#include "third_party/logle/gtest.h"

namespace tervuren {
namespace {

TEST(AccountAccessGraphDeathTest, UninitializedCall) {
  AccountAccessGraph graph;
  EXPECT_DEATH({ graph.ProcessAccessData(unordered_map<string, int>(), {}); },
               ".*");
}

class AccountAccessGraphTest : public ::testing::Test {
 protected:
  void SetUp() override {
    util::Status s = graph_.Initialize();
    EXPECT_TRUE(s.ok());
  }

  AccountAccessGraph graph_;
};

static vector<string> fields1{"bad-person@logle", "manager-person@logle",
                              "Bad Actor", "32", "user@fake-mail"};

static vector<string> fields2{"good-person@logle", "manager-person@logle",
                              "Good Actor", "32", "user@logle-mail"};

unordered_map<string, int> GetIndex() {
  unordered_map<string, int> index;
  index.insert({access::kActor, 0});
  index.insert({access::kActorManager, 1});
  index.insert({access::kActorTitle, 2});
  index.insert({access::kNumAccesses, 3});
  index.insert({access::kUser, 4});
  return index;
}

TEST_F(AccountAccessGraphTest, ProcessSingleAccess) {
  EXPECT_EQ(0, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
  graph_.ProcessAccessData(GetIndex(), fields1);
  EXPECT_EQ(2, graph_.NumNodes());
  EXPECT_EQ(1, graph_.NumEdges());
  // Processing the same data multiple times will not change the graph.
  graph_.ProcessAccessData(GetIndex(), fields1);
  EXPECT_EQ(2, graph_.NumNodes());
  EXPECT_EQ(1, graph_.NumEdges());
  // Changing the user but not the actor should add only one new node to the
  // graph.
  fields1[4] = "user2@fake-mail";
  graph_.ProcessAccessData(GetIndex(), fields1);
  EXPECT_EQ(3, graph_.NumNodes());
  EXPECT_EQ(2, graph_.NumEdges());
  // Changing the actor but not the user should also only one new node to the
  // graph.
  fields1[0] = "another-actor@logle";
  graph_.ProcessAccessData(GetIndex(), fields1);
  EXPECT_EQ(4, graph_.NumNodes());
  EXPECT_EQ(3, graph_.NumEdges());
}

}  // namespace
}  // namespace tervuren
