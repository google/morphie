// An analyzer for visualizing relationships between actors and accounts they
// access. The input data to this analyzer provides details about an actor and
// the accounts they access.  The output of this analyzer is a visualization of
// this data.
#ifndef THIRD_PARTY_LOGLE_ACCOUNT_ACCESS_ANALYZER_H_
#define THIRD_PARTY_LOGLE_ACCOUNT_ACCESS_ANALYZER_H_

#include <memory>
#include <unordered_map>

#include "third_party/logle/account_access_graph.h"
#include "third_party/logle/base/string.h"
#include "third_party/logle/util/csv.h"
#include "third_party/logle/util/status.h"

namespace third_party_logle {

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

}  // namespace third_party_logle

#endif  // THIRD_PARTY_LOGLE_ACCOUNT_ACCESS_ANALYZER_H_
