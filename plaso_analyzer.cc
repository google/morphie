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

#include "plaso_analyzer.h"

#include <boost/algorithm/string/join.hpp>  // NOLINT

#include <set>
#include <utility>

#include "base/vector.h"
#include "json_reader.h"
#include "plaso_defs.h"
#include "plaso_event.h"
#include "util/logging.h"
#include "util/status.h"
#include "util/string_utils.h"

namespace {

const int kMaxMalformedLines = 1000000;

}  // namespace

namespace tervuren {

util::Status PlasoAnalyzer::Initialize(
    JsonDocumentIterator* doc_iterator) {
  CHECK(doc_iterator != nullptr, "The pointer to the JSON document is null.");
  this-> doc_iterator_ = doc_iterator;
  return util::Status::OK;
}

void PlasoAnalyzer::BuildPlasoGraph() {
  plaso_graph_.reset(new PlasoEventGraph(show_all_sources_));
  if (!plaso_graph_->Initialize().ok()) {
    plaso_graph_.reset(nullptr);
    return;
  }
  return BuildPlasoGraphFromJSON();
}

string PlasoAnalyzer::PlasoGraphDot() const {
  return (plaso_graph_ == nullptr) ? "" : plaso_graph_->ToDot();
}

string PlasoAnalyzer::PlasoGraphPbTxt() const {
  return (plaso_graph_ == nullptr) ? "" : plaso_graph_->ToPbTxt();
}

string PlasoAnalyzer::PlasoGraphStats() const {
  if (plaso_graph_ == nullptr) {
    return "Graph has not been created!";
  }
  return plaso_graph_->GetStats();
}

void PlasoAnalyzer::IncrementSkipCounter() {
  ++num_lines_skipped_;
  CHECK(num_lines_skipped_ < kMaxMalformedLines,
        "Over a million malformed lines in input. Aborting.");
}

void PlasoAnalyzer::BuildPlasoGraphFromJSON() {
  const set<string> required_fields =
      util::SplitToSet(plaso::kRequiredFields, ',');
  CHECK(!required_fields.empty(), "No required fields in input.");
  // List of all event names.
  // This variable will point to the data for a single event.
  const Json::Value* json_event;
  // This proto will contain fields extracted from '*json_event'.
  PlasoEvent event_data;
  bool has_all_fields;

  while (this->doc_iterator_->HasNext()) {
    json_event = this->doc_iterator_->Next();
    CHECK(json_event != nullptr, "json_event is null!");
    has_all_fields = std::all_of(required_fields.begin(), required_fields.end(),
                                 [json_event](const string& field) {
                                   return json_event->isMember(field);
                                 });
    if (!has_all_fields) {
      IncrementSkipCounter();
      continue;
    }
    event_data = plaso::ParseJSON(*json_event);
    plaso_graph_->ProcessEvent(event_data);
  }
  plaso_graph_->AddTemporalEdges();
}

}  // namespace tervuren
