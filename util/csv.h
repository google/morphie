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

// This file defines a simple parser for Comma Separated Values and a Record
// that stores values extracted from a single line of input. Both the parser and
// the record support range-based iterartion. The parser uses the double-quote
// symbol as an escape character and backslash to escape the quote symbol.
//
// Example 1.
//   // Assume 'data' is a string.
//   auto input = new std::stringstream(data);
//   CSVParser parser(input);
//   for (const Record& record : parser) {
//     if (!record.ok()) continue;
//     for (const string& field : record) {
//       // Use the field.
//     }
//   }
//
// Example 2.
//   string data = R"(5,",",,"\"")";
//   auto input = new std::stringstream(data);
//   CSVParser parser(input);
//   auto record_it = parser.begin()->begin();
//   // A digit.
//   EXPECT_EQ("5", *record_it);
//   ++record_it;
//   // A quoted comma.
//   EXPECT_EQ(",", *record_it);
//   ++record_it;
//   // An empty quoted field.
//   EXPECT_EQ("", *record_it);
//   // An escaped quotation symbol.
//   ++record_it;
//   EXPECT_EQ("\"", *record_it);
//
// The parser currently allows for a delimiter to be provided as input but fixes
// the escape character to be the quote symbol " and backslash for escaping
// the quote symbol.
#ifndef THIRD_PARTY_LOGLE_UTIL_CSV_H_
#define THIRD_PARTY_LOGLE_UTIL_CSV_H_

#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

#include "third_party/logle/base/string.h"
#include "third_party/logle/util/status.h"

namespace tervuren {
namespace util {

using std::vector;

// A Record object consists of a vector of strings and a status object. If the
// status is ok(), the vector contains fields obtained by parsing one line of
// CSV input. If the status is not ok(), an error occurred when the Record was
// being populated. Both fields and status are set by the CSV parser and cannot
// be modified by the client.
class Record {
 public:
  // Functions that enable range-based iteration over fields.
  vector<string>::const_iterator begin() const { return fields_.begin(); }
  vector<string>::const_iterator end() const { return fields_.end(); }

  const vector<string>& fields() const { return fields_; }
  bool ok() const { return status_.ok(); }

 private:
  friend class CSVParser;
  vector<string> fields_;
  util::Status status_;
};

// The CSVParser extracts fields, line by line, from an input stream. At any
// given time, the parser only stores fields extracted from a single line in
// memory, so processing CSV input with a large number of lines is not an issue.
//
// The parser owns the input stream and provides an iterator interface for
// processing the stream. There can only be one, non-end position.
class CSVParser {
 public:
  // An iterator class for traversing the processed input stream.
  class Iterator : public std::iterator<std::input_iterator_tag, Record> {
   public:
    // The field 'is_end' is used internally to determine if an iterator points
    // to the end of a stream.
    explicit Iterator(CSVParser* parser, bool is_end)
        : parser_(parser), is_end_(is_end) {}

    // Parse the next line of the input. Does nothing if already at the end.
    Iterator& operator++();

    const Record& operator*() { return parser_->record_; }
    const Record* operator->() { return &parser_->record_; }

    // Two iterators are equal if they are defined by the same parser and both
    // are end iterators or neither is an end iterator. Since the parser only
    // provides accesse to one non-end position in the stream, two non-end
    // iterators defined by the same parser must be equal.
    bool operator==(const Iterator& that) const {
      // There are two ways in which an iterator might point to the end of the
      // input. One is that it was created as an end iterator, in which case the
      // 'is_end_' flag is true. Another way is by incrementing an iterator from
      // parser.begin() till the end. In this case, the 'is_end_' flag will be
      // initialized to false, and the end position is detected by checking that
      // there is no more data. The checks below cannot be simplified by
      // updating the value of 'is_end_' in an iterator because there may be
      // multiple iterators that point to non-end positions.
      bool this_at_end = is_end_ || parser_->state_ == State::kOutputEmpty;
      bool that_at_end =
          that.is_end_ || that.parser_->state_ == State::kOutputEmpty;
      return (parser_ == that.parser_ && this_at_end == that_at_end);
    }

    bool operator!=(const Iterator& that) const { return !(*this == that); }

   private:
    CSVParser* const parser_;
    bool is_end_;
  };

  // The CSVParser takes ownership of 'input', consumes its contents and
  // eventually destroys it.
  explicit CSVParser(std::istream* input);
  // Creates a CSVParser that uses 'delim' as the field delimiter.
  CSVParser(std::istream* input, char delim);

  Iterator begin() const { return begin_iter_; }
  Iterator end() const { return end_iter_; }

 private:
  // The state of the parser. The parser is one step ahead of the client reading
  // parsed fields, so it needs to track when the input has been exhaused but
  // the last Record extracted from the input has not yet been read.
  enum class State {
    // Reading and parsing input.
    kReading,
    // Raw input is exhaused but there is one Record in the parsed input.
    kInputEmpty,
    // All parsed input has been consumed.
    kOutputEmpty,
  };

  void Init(std::istream* istream);
  // Parses the next line of the input and updates the parser's state.
  void Advance();

  std::unique_ptr<std::istream> input_;
  const char delim_;
  Record record_;
  Iterator begin_iter_;
  Iterator end_iter_;
  State state_;
};

}  // namespace util
}  // namespace tervuren

#endif  // THIRD_PARTY_LOGLE_UTIL_CSV_H_
