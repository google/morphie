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

#include "analyzers/examples/account_access_analyzer.h"

#include <boost/algorithm/string/join.hpp>  // NOLINT

#include <algorithm>
#include <set>
#include <utility>

#include "analyzers/examples/account_access_defs.h"
#include "base/vector.h"
#include "util/logging.h"
#include "util/string_utils.h"

namespace {

const int kMaxMalformedLines = 1000000;
const char kNullAccessGraphErr[] = "The access graph is null.";

}  // namespace

namespace morphie {
// This function performs a series of checks, described by the error message
// generated if the check fails.
util::Status AccessAnalyzer::Initialize(
    std::unique_ptr<util::CSVParser> parser) {
  csv_parser_ = std::move(parser);
  auto row_it = csv_parser_->begin();
  if (row_it == csv_parser_->end()) {
    return util::Status(Code::INVALID_ARGUMENT, "The input is empty.");
  }
  util::Status status = InitializeFieldMap();
  if (!status.ok()) {
    return status;
  }
  ++row_it;
  if (row_it == csv_parser_->end()) {
    return util::Status(Code::INVALID_ARGUMENT, "No data in the input.");
  }
  return util::Status::OK;
}

int AccessAnalyzer::NumGraphNodes() const {
  CHECK(access_graph_ != nullptr, kNullAccessGraphErr);
  return access_graph_->NumNodes();
}

int AccessAnalyzer::NumGraphEdges() const {
  CHECK(access_graph_ != nullptr, kNullAccessGraphErr);
  return access_graph_->NumEdges();
}

util::Status AccessAnalyzer::BuildAccessGraph() {
  if (csv_parser_ == nullptr) {
    return util::Status(Code::INVALID_ARGUMENT,
                        "The CSV parser has not been initialized.");
  }
  if (access_graph_ != nullptr) {
    return util::Status(Code::INVALID_ARGUMENT,
                        "The access graph has already been created.");
  }
  access_graph_.reset(new AccountAccessGraph);
  util::Status status = access_graph_->Initialize();
  if (!status.ok()) {
    access_graph_.reset(nullptr);
    return status;
  }
  for (const util::Record& record : *csv_parser_) {
    ++num_lines_read_;
    if (record.fields().size() != field_to_index_.size()) {
      IncrementSkipCounter();
      continue;
    }
    access_graph_->ProcessAccessData(field_to_index_, record.fields());
  }
  return util::Status::OK;
}

string AccessAnalyzer::AccessGraphAsDot() const {
  return (access_graph_ == nullptr) ? "" : access_graph_->ToDot();
}

void AccessAnalyzer::IncrementSkipCounter() {
  ++num_lines_skipped_;
  CHECK(num_lines_skipped_ < kMaxMalformedLines,
        util::StrCat("Over ", std::to_string(kMaxMalformedLines),
                     " malformed lines in input. Aborting."));
}

util::Status AccessAnalyzer::InitializeFieldMap() {
  const vector<string>& field_names = csv_parser_->begin()->fields();
  if (field_names.empty()) {
    return util::Status(Code::INVALID_ARGUMENT, "First line has no columns.");
  }
  int index = 0;
  set<string> provided_fields;
  for (const string& field_name : field_names) {
    if (field_name.empty()) {
      return util::Status(
          Code::INVALID_ARGUMENT,
          util::StrCat("Column number ", std::to_string(index + 1),
                       " has no name."));
    }
    auto insert_status = field_to_index_.insert({field_name, index});
    if (!insert_status.second) {
      return util::Status(
          Code::INVALID_ARGUMENT,
          util::StrCat("More than one column named:", field_name));
    }
    provided_fields.insert(field_name);
    ++index;
  }
  const set<string> required_fields =
      util::SplitToSet(access::kRequiredFields, ',');
  set<string> missing_fields;
  std::set_difference(required_fields.begin(), required_fields.end(),
                      provided_fields.begin(), provided_fields.end(),
                      std::inserter(missing_fields, missing_fields.begin()));
  if (missing_fields.size() > 0) {
    string fields = util::SetJoin(missing_fields, ",");
    return util::Status(
        Code::INVALID_ARGUMENT,
        util::StrCat("The following required fields are missing: ", fields));
  }
  return util::Status::OK;
}

}  // namespace morphie
