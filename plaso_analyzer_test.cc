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

#include "third_party/logle/plaso_analyzer.h"

#include <sstream>
#include <vector>

#include "third_party/logle/base/string.h"
#include "third_party/logle/gtest.h"
#include "third_party/logle/plaso_defs.h"
#include "third_party/logle/util/string_utils.h"

namespace third_party_logle {

namespace {

const char kFieldNames[] =
    "datetime,timestamp_desc,data_type,message,display_name";
const char kData[] =
    "\n2012-04-03T00:25:21+00:00,ctime,"
    "windows:registry:key_value,"
    "filestat,"
    "K:/Documents and Settings/user/Application Data/Dropbox/l/3xp701t";
// This string is used to exercise the invalid timestamp condition
const char kNonData[] = "\n,,,,";

// Tests of JSON input validation and input processing.
void TestJSONInitialization(const string& kInput, Code code) {
  std::unique_ptr<::Json::Value> doc(new ::Json::Value);
  ::Json::Reader reader;
  ASSERT_TRUE(reader.parse(kInput, *doc, false /*Ignore comments.*/));
  PlasoAnalyzer analyzer;
  util::Status s = analyzer.Initialize(std::move(doc));
  EXPECT_EQ(code, s.code());
}

// The next few tests below check that the analyzer cannot be initialized with
// invalid input.
TEST(PlasoAnalyzerDeathTest, RequiresNonNullJSONDoc) {
  std::unique_ptr<::Json::Value> doc;
  PlasoAnalyzer analyzer;
  EXPECT_DEATH(
      { util::Status s = analyzer.Initialize(std::move(doc)); },
      ".*null.*");
}

TEST(PlasoAnalyzerTest, RejectsInvalidJSONInput) {
  TestJSONInitialization("{}", Code::INVALID_ARGUMENT);
  TestJSONInitialization("[]", Code::INVALID_ARGUMENT);
  TestJSONInitialization(R"({"foo" : {}})", Code::INVALID_ARGUMENT);
  TestJSONInitialization(R"({"hits" : {}})", Code::INVALID_ARGUMENT);
  TestJSONInitialization(R"({ "hits" : { "foo" : [] } })",
                         Code::INVALID_ARGUMENT);
  TestJSONInitialization(R"({ "hits" : { "hits" : [] } })",
                         Code::INVALID_ARGUMENT);
  // Initialization expects a non-empty array but the contents of the array are
  // only validated during graph construction.
  TestJSONInitialization(R"({ "hits" : { "hits" : [ "a", "b", "c"] } })",
                         Code::OK);
  TestJSONInitialization(
      R"({ "hits" : { "hits" : [
  { "datetime" : "1970-01-01T00:00:00+00:00"},
  { "datetime" : "1970-01-01T00:00:00+00:00"}
] } })",
      Code::OK);
}

}  // namespace
}  // namespace third_party_logle
