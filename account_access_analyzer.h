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

// An analyzer for visualizing relationships between actors and accounts they
// access. The input data to this analyzer provides details about an actor and
// the accounts they access.  The output of this analyzer is a visualization of
// this data.
#ifndef LOGLE_ACCOUNT_ACCESS_ANALYZER_H_
#define LOGLE_ACCOUNT_ACCESS_ANALYZER_H_

#include <memory>
#include <unordered_map>

#include "account_access_graph.h"
#include "base/string.h"
#include "util/csv.h"
#include "util/status.h"

namespace logle {

// The AccessAnalyzer is initialized with a CSV parser. The fields that must be
// present in the CSV input are defined in account_access_defs.h.
class AccessAnalyzer {
 public:
  AccessAnalyzer() : num_lines_read_(0), num_lines_skipped_(0) {}

  // Initializes the analyzer using a CSV parser. Returns
  //  * OK : if the following requirements are satisfied.
  //    - The input contains the fields in access::kRequiredFields.
  //    - There is at least one row of data after the CSV header.
  //  * INVALID_ARGUMENT : otherwise with the error message containing the
  //  reason why initialization failed.
  util::Status Initialize(std::unique_ptr<util::CSVParser> parser);

  util::Status BuildAccessGraph();

  // Utilities for accounting and error checking.
  int NumLinesRead() const { return num_lines_read_; }
  int NumLinesSkipped() const { return num_lines_skipped_; }
  int NumLinesProcessed() const { return num_lines_read_ - num_lines_skipped_; }

  // Statistics about the account access graph. These functions require that the
  // access graph has been created and crash otherwise.
  int NumGraphNodes() const;
  int NumGraphEdges() const;

  // Returns the account access graph in GraphViz DOT format.
  string AccessGraphAsDot() const;

 private:
  void IncrementSkipCounter();
  // Initializes field_to_index_ using the first line of csv_parser_.
  util::Status InitializeFieldMap();

  // A map from input field names to the input column with that data.
  unordered_map<string, int> field_to_index_;
  std::unique_ptr<AccountAccessGraph> access_graph_;

  int num_lines_read_;
  int num_lines_skipped_;
  std::unique_ptr<util::CSVParser> csv_parser_;
};

}  // logle

#endif  // LOGLE_ACCOUNT_ACCESS_ANALYZER_H_
