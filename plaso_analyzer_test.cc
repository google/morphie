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

#include <stdio.h>

#include "third_party/logle/base/string.h"
#include "third_party/logle/json_reader.h"
#include "third_party/logle/gtest.h"
#include "third_party/logle/plaso_analyzer.h"
#include "third_party/logle/util/status.h"

namespace third_party_logle {
namespace {
namespace logle = third_party_logle;
namespace util = third_party_logle::util;

// Tests of JSON input validation and input processing.
// NOLINTNEXTLINE
string json_object = R"({"event1":{"data_type": "fs:stat", "display_name": "GZIP:/usr/share/info/bc.info.gz", "timestamp": 0, "timestamp_desc": "mtime" }
, "event2":{"display_name": "GZIP:/usr/share/info/bc.info.gz", "data_type": "fs:stat", "timestamp": 0, "timestamp_desc": "mtime" }
, "event3":{"display_name": "GZIP:/usr/share/info/coreutils.info.gz", "data_type": "fs:stat", "timestamp": 0, "timestamp_desc": "mtime"}
})";

// NOLINTNEXTLINE
string json_stream = R"({"data_type": "fs:stat", "display_name": "GZIP:/usr/share/info/bc.info.gz", "timestamp": 0, "timestamp_desc": "mtime" }
{"display_name": "GZIP:/usr/share/info/bc.info.gz", "data_type": "fs:stat", "timestamp": 0, "timestamp_desc": "mtime" }
{"display_name": "GZIP:/usr/share/info/coreutils.info.gz", "data_type": "fs:stat", "timestamp": 0, "timestamp_desc": "mtime"})";

void TestInitialization(
    const string& file_content, bool is_line_json) {
  PlasoAnalyzer analyzer;
  istringstream stream(file_content);
  util::Status status;

  if (is_line_json) {
    logle::StreamJson jstream(&stream);
    status = analyzer.Initialize(&jstream);
    analyzer.BuildPlasoGraph();
  } else {
    logle::FullJson jstream(&stream);
    status = analyzer.Initialize(&jstream);
    analyzer.BuildPlasoGraph();
  }
  analyzer.PlasoGraphDot();
}

// Basic testing for correct JSON input files.
TEST(PlasoAnalyzerTest, AcceptValidJSONInput) {
  TestInitialization(json_object, false);
  TestInitialization(json_stream, true);
}

// Basic testing for incorrect JSON input files.
TEST(PlasoAnalyzerDeathTest, RequiresCorrectJSONDoc) {
  std::unique_ptr<::Json::Value> doc;
  PlasoAnalyzer analyzer;
  EXPECT_DEATH(TestInitialization(json_object, true),
    ".*Line is not in JSON format.*");
  EXPECT_DEATH(TestInitialization(json_stream, false),
    ".*JSON*");
}
}  // namespace
}  // namespace third_party_logle
