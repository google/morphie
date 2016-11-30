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

// An analyzer for Plaso logs. Plaso is a forensic tool that generates
// semi-structured output called a super-timeline, which consists of a
// timestamped sequence of events. This analyzer operates on a supertimeline
// represented as a JSON document. The output of the analysis is either a
// visualization of log data or the results of an analysis.
#ifndef LOGLE_PLASO_ANALYZER_H_
#define LOGLE_PLASO_ANALYZER_H_

#include <algorithm>
#include <memory>
#include <unordered_map>

#include "analyzers/plaso/plaso_event_graph.h"
#include "base/string.h"
#include "json/json.h"
#include "util/json_reader.h"
#include "util/status.h"

namespace morphie {

// The PlasoAnalyzer uses graphs to extract information from log data. The
// analyzer must be initialized with a JSON document (a Json::Value object from
// JSONCpp). The document must satisfy constraints described in the comments
// above the Initialize function.
class PlasoAnalyzer {
 public:
  explicit PlasoAnalyzer(bool show_all_sources)
      : show_all_sources_(show_all_sources),
        num_lines_read_(0),
        num_lines_skipped_(0) {}

  // Initializes the log analyzer with a JSON document.
  //  * Requires that 'json_doc' is not null.
  //  * Returns OK if the JSON document is an object containing key-value pairs.
  //  * Returns INVALID_ARGUMENT otherwise.
  // The document structure is defined by the data source not by logle. The keys
  // in the object specify an event identifier and the value is another object
  // containing event data. Additional error validation is done
  // during graph construction.
  util::Status Initialize(JsonDocumentIterator* json_doc);

  // Constructs a PlasoEventGraph (defined in plaso_event_graph.h) from the
  // input data. Requires that the analyzer has been initialized and that every
  // object in the JSON input contains the fields listed in the documentation of
  // the Initialize function above.
  void BuildPlasoGraph();

  // Utilities for accounting and error checking.
  int NumLinesRead() { return num_lines_read_; }
  int NumLinesSkipped() { return num_lines_skipped_; }
  int NumLinesProcessed() { return num_lines_read_ - num_lines_skipped_; }

  int NumNodes() {
    return (plaso_graph_ == nullptr) ? 0 : plaso_graph_->NumNodes();
  }

  int NumEdges() {
    return (plaso_graph_ == nullptr) ? 0 : plaso_graph_->NumEdges();
  }

  string PlasoGraphStats() const;
  string PlasoGraphDot() const;
  string PlasoGraphPbTxt() const;

 private:
  // Constructs a Plaso graph using a JSON document.
  void BuildPlasoGraphFromJSON();
  // The skip counter tracks the number of the serialized event objects in the
  // input that were skipped.
  void IncrementSkipCounter();

  // Configuration options for the analyzer.
  bool show_all_sources_;

  // Data about analyzer state.
  std::unique_ptr<PlasoEventGraph> plaso_graph_;
  int num_lines_read_;
  int num_lines_skipped_;
  JsonDocumentIterator* doc_iterator_;
};

}  // namespace morphie

#endif  // LOGLE_PLASO_ANALYZER_H_
