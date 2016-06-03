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
#include <vector>

#include "plaso_defs.h"
#include "plaso_event.h"
#include "util/logging.h"
#include "util/string_utils.h"

namespace {

const int kMaxMalformedLines = 1000000;

}  // namespace

namespace logle {

util::Status PlasoAnalyzer::Initialize(
    std::unique_ptr<::Json::Value> json_doc) {
  CHECK(json_doc != nullptr, "The pointer to the JSON document is null.");
  json_doc_ = std::move(json_doc);
  if (!json_doc_->isObject()
      || !json_doc_->isMember("hits")
      || !(*json_doc_)["hits"].isObject()
      || !(*json_doc_)["hits"].isMember("hits")
      || !((*json_doc_)["hits"])["hits"].isArray()) {
    return util::Status(Code::INVALID_ARGUMENT,
                        "The JSON doc does not have the expected format.");
  }
  if (((*json_doc_)["hits"])["hits"].empty()) {
    return util::Status(Code::INVALID_ARGUMENT, "No data in the input.");
  }
  return util::Status::OK;
}

void PlasoAnalyzer::BuildPlasoGraph() {
  CHECK(json_doc_ != nullptr, "No input provided");
  plaso_graph_.reset(new PlasoEventGraph);
  if (!plaso_graph_->Initialize().ok()) {
    plaso_graph_.reset(nullptr);
    return;
  }
  return BuildPlasoGraphFromJSON();
}

string PlasoAnalyzer::PlasoGraphDot() const {
  return (plaso_graph_ == nullptr) ? "" : plaso_graph_->ToDot();
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
  // This variable will point to the data for a single event.
  Json::Value* json_event;
  // This proto will contain fields extracted from '*json_event'.
  PlasoEvent event_data;
  bool has_all_fields;
  for (auto event_it = ((*json_doc_)["hits"])["hits"].begin();
       event_it != ((*json_doc_)["hits"])["hits"].end(); ++event_it) {
    if (event_it->isObject() && event_it->isMember("_source")) {
      json_event = &((*event_it)["_source"]);
      CHECK(json_event != nullptr, "json_event is null!");
      has_all_fields =
          std::all_of(required_fields.begin(),
                      required_fields.end(),
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
  }
  plaso_graph_->AddTemporalEdges();
}

}  // namespace logle
