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

// An analyzer for Curio streams. The analyzer uses graphs to represent
// relationships between Curio streams. These graphs can then be used for
// analysis and visualization.
#ifndef THIRD_PARTY_LOGLE_CURIO_ANALYZER_H_
#define THIRD_PARTY_LOGLE_CURIO_ANALYZER_H_

#include <memory>

#include "third_party/jsoncpp/json.h"
#include "third_party/logle/base/string.h"
#include "third_party/logle/stream_dependency_graph.h"
#include "third_party/logle/util/status.h"

namespace third_party_logle {

// The CurioAnalyzer ingests stream definitions provided in JSON and calls the
// appropriate APIs for graph construction and visualization. Initialize() must
// be called before any other function is called or those calls will crash.
class CurioAnalyzer {
 public:
  CurioAnalyzer() : num_streams_processed_(0), num_streams_skipped_(0) {}

  // Initializes the analyzer with a JSON document and resets the counters
  // that track the number of streams processed and skipped.
  //  * Returns OK if 'json_doc' satisfies these conditions:
  //    - It must not be null.
  //    - It must not be an empty JSON document.
  //    - It must be an object.
  //  * Returns INVALID_ARGUMENT otherwise with an error message.
  util::Status Initialize(std::unique_ptr<::Json::Value> json_doc);

  // Constructs a StreamDependencyGraph object from the JSON document provided
  // to Initialize.
  util::Status BuildDependencyGraph();

  int NumStreamsProcessed() const { return num_streams_processed_; }
  int NumStreamsSkipped() const { return num_streams_skipped_; }

  int NumGraphNodes() const;
  int NumGraphEdges() const;

  // Returns a GraphViz DOT representation of the dependency graph.
  string DependencyGraphAsDot() const;

 private:
  // Recursively adds nodes and edges to the dependency graph for each stream in
  // 'consumer_tree'. The 'consumer_tree' is assumed to contain only one JSON
  // object at the top level. Returns:
  //  - INVALID_ARGUMENT : if the number of malformed stream definitions exceeds
  //    a predefined threshold (kMaxMalformedObjects in curio_analyzer.cc).
  //  - OK : otherwise.
  util::Status AddDependencies(const string& consumer_id,
                               const Json::Value& consumer_tree);

  // Increments a global counter of the number of objects in the JSON input that
  // have been skipped. Returns
  //  - INVALID_ARGUMENT if the result of the increment exceeds the threshold
  //    defined by kMaxMalformedObjects in curio_analyzer.cc.
  //  - OK otherwise.
  util::Status IncrementSkipCounter();

  int num_streams_processed_;
  int num_streams_skipped_;

  std::unique_ptr<Json::Value> json_doc_;
  std::unique_ptr<StreamDependencyGraph> dependency_graph_;
};

}  // namespace third_party_logle

#endif  // THIRD_PARTY_LOGLE_CURIO_ANALYZER_H_
