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
#include "util/csv.h"

#include <boost/tokenizer.hpp>  // NOLINT
#include <boost/exception/exception.hpp>  // NOLINT
#include <sstream>

// The Clang compiler does not support exceptions. Boost is compiled with Clang,
// the client is required to define this function.
// void boost::throw_exception(std::exception const& e) {}


namespace morphie {
namespace util {

CSVParser::Iterator& CSVParser::Iterator::operator++() {
  parser_->Advance();
  return *this;
}

CSVParser::CSVParser(std::istream* input) : CSVParser(input, ',') {}

CSVParser::CSVParser(std::istream* input, char delim)
    : delim_(delim),
      begin_iter_(this, false),
      end_iter_(this, true),
      state_(State::kReading) {
  Init(input);
}

void CSVParser::Init(std::istream* input) {
  if (input == nullptr || !input->good()) {
    input_.reset(nullptr);
    state_ = State::kOutputEmpty;
    record_.status_ = util::Status(Code::INVALID_ARGUMENT, "Input is null.");
    return;
  }
  input_.reset(input);
  Advance();
}

// The parser begins in the kReading state and makes the following state
// transitions.
//  - kReading -> kInputEmpty if there is exactly one line left in the input.
//  - kReading -> kReading if there is more than one line in the input. In both
//    transitions out of kReading, the next input line is parsed, so the output
//    is not empty.
//  - kInputEmpty -> kOutputEmpty.
//  - kOutputEmpty-> kOutputEmpty.
// The transitions from kInputEmpty and kOutputEmpty are not based on other
// conditions.
void CSVParser::Advance() {
  record_.fields_.clear();
  switch (state_) {
    case State::kOutputEmpty:
      return;
    case State::kInputEmpty: {
      state_ = State::kOutputEmpty;
      return;
    }
    case State::kReading: {
      string line;
      std::getline(*input_, line);
      try {
        boost::escaped_list_separator<char> sep("\\", string(1, delim_), "\"");
        boost::tokenizer<boost::escaped_list_separator<char>> tokenizer(line,
                                                                        sep);
        record_.fields_.assign(tokenizer.begin(), tokenizer.end());
      } catch (boost::exception& e) {
        record_.status_ =
            util::Status(Code::INVALID_ARGUMENT, "Error tokenizing line.");
      }
      if (!input_->good()) {
        input_.reset(nullptr);
        state_ = State::kInputEmpty;
      }
    }
  }
}

}  // namespace util
}  // namespace morphie
