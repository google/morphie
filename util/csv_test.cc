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

#include <sstream>

#include "gtest.h"

namespace tervuren {
namespace util {
namespace {

// Exercise the API of the Record class.
TEST(CSVTest, RecordAPI) {
  Record r;
  EXPECT_TRUE(r.ok());
  EXPECT_EQ(r.begin(), r.end());
  EXPECT_TRUE(r.fields().empty());
}

TEST(CSVTest, Initialization) {
  // The null stream is accepted but contains no records.
  CSVParser null_parser(nullptr);
  EXPECT_EQ(null_parser.begin(), null_parser.end());
  // The stream with an empty string contains one, empty record.
  auto empty_stream = new std::stringstream("");
  CSVParser parser(empty_stream);
  CSVParser::Iterator csv_it = parser.begin();
  // Test the pointer and dereference operators.
  EXPECT_TRUE(csv_it->ok());
  EXPECT_TRUE((*csv_it).ok());
}

// Iterators from distinct parsers, even with the same input, are incomparable.
TEST(CSVTest, DistinctParsers) {
  auto s1 = new std::stringstream("");
  auto s2 = new std::stringstream("");
  CSVParser parser1(s1);
  CSVParser parser2(s2);
  EXPECT_NE(parser1.begin(), parser2.begin());
  EXPECT_NE(parser1.end(), parser2.end());
}

// Equality and increment tests for end iterators.
TEST(CSVTest, IteratorEnd) {
  // This input contains one field, the empty string.
  auto s = new std::stringstream("");
  CSVParser parser(s);
  auto csv_it = parser.begin();
  EXPECT_NE(csv_it, parser.end());
  ++csv_it;
  EXPECT_EQ(csv_it, parser.end());
  // The pre-increment operator does nothing once the end position is reached.
  ++csv_it;
  EXPECT_EQ(csv_it, parser.end());
  ++csv_it;
  EXPECT_EQ(csv_it, parser.end());
}

// Multiple iterators to non-end positions are all equal. Moreover, updating one
// will update all of them.
TEST(CSVTest, MultipleIterators) {
  // An iterator to this input reaches the end after two increments.
  auto s = new std::stringstream("a\nb");
  CSVParser parser(s);
  // The first and second iterator below should always be equal.
  auto it1 = parser.begin(), it2 = parser.begin(), it3 = parser.end();
  EXPECT_NE(it1, it3);
  EXPECT_NE(it2, it3);
  // Only the first iterator is incremented but both will be equal.
  ++it1;
  EXPECT_EQ(it1, it2);
  EXPECT_NE(it1, it3);
  EXPECT_NE(it2, it3);
  // Only the second iterator is incremented. Since the input is entirely
  // consumed, all iterators will now be equal.
  ++it2;
  EXPECT_EQ(it1, it2);
  EXPECT_EQ(it1, it3);
  EXPECT_EQ(it2, it3);
}

TEST(CSVTest, ParseEmptyStrings) {
  // A single empty string creates an empty record.
  auto es = new std::stringstream("");
  CSVParser p(es);
  EXPECT_NE(p.begin(), p.end());
  EXPECT_TRUE(p.begin()->fields().empty());
  EXPECT_EQ(0, p.begin()->fields().size());
  // The comma character should be parsed as two empty strings.
  es = new std::stringstream(",");
  CSVParser q(es);
  EXPECT_EQ(2, q.begin()->fields().size());
  // Test the range-based iteration of Record as well as the parsed contents.
  for (const auto& field : *q.begin()) {
    EXPECT_EQ("", field);
  }
  // Test the range-based iteration of CSVParser on an input with three lines
  // and two fields on each line.
  es = new std::stringstream(",\n,\n,");
  CSVParser r(es);
  int num_lines = 0;
  for (const auto& record : r) {
    EXPECT_EQ(2, record.fields().size());
    EXPECT_TRUE(record.ok());
    int num_fields = 0;
    for (const auto& field : record) {
      EXPECT_EQ("", field);
      ++num_fields;
    }
    EXPECT_EQ(2, num_fields);
    ++num_lines;
  }
  EXPECT_EQ(3, num_lines);
}

TEST(CSVTest, ParseOneLine) {
  auto ss = new std::stringstream("a,b,c");
  CSVParser parser(ss);
  EXPECT_EQ(3, parser.begin()->fields().size());
  auto record_it = parser.begin()->begin();
  EXPECT_EQ("a", *record_it);
  ++record_it;
  EXPECT_EQ("b", *record_it);
  ++record_it;
  EXPECT_EQ("c", *record_it);
}

// This function tests that parsing 'input' with the provided delimiter produces
// the sequence of fields in 'results'.
void TestParser(char delim, const string& input,
                const std::vector<std::vector<string>>& results) {
  auto ss = new std::stringstream(input);
  CSVParser parser(ss, delim);
  auto record_it = parser.begin();
  auto result_it = results.begin();
  for (; record_it != parser.end() && result_it != results.end();
       ++record_it, ++result_it) {
    auto field_it = record_it->begin();
    auto rfield_it = result_it->begin();
    for (; field_it != record_it->end() && rfield_it != result_it->end();
         ++field_it, ++rfield_it) {
      EXPECT_EQ(*field_it, *rfield_it);
    }
  }
}

TEST(CSVTest, MultiLineSameFields) {
  std::vector<std::vector<string>> results;
  results.push_back({"a", "b", "c"});
  results.push_back({"aa", "bb", "cc"});
  results.push_back({"aaa", "bbb", "ccc"});
  string input = "a,b,c\naa,bb,cc\naaa,bbb,ccc";
  TestParser(',', input, results);
}

// The CSV parser does not enforce that the same number of fields should occur
// on each line of the input.
TEST(CSVTest, MultiLinesDifferentFields) {
  std::vector<std::vector<string>> results;
  results.push_back({"a"});
  results.push_back({"aa", "bb"});
  results.push_back({"aaa", "bbb", "ccc"});
  results.push_back({"aaaa", "bbbb", "cccc", "dddd"});
  results.push_back({"aaa", "bbb", "ccc"});
  results.push_back({"aa", "bb"});
  results.push_back({"a"});
  string input =
      "a\naa,bb\naaa,bbb,ccc\naaaa,bbbb,cccc,dddd\naaa,bbb,ccc\naa,bb\na";
  TestParser(',', input, results);
}

TEST(CSVTest, MultiLinesPeriodDelimiter) {
  std::vector<std::vector<string>> results;
  results.push_back({"a"});
  results.push_back({"aa", "bb"});
  results.push_back({"aaa", "bbb", "ccc"});
  results.push_back({"aaaa", "bbbb", "cccc", "dddd"});
  results.push_back({"aaa", "bbb", "ccc"});
  results.push_back({"aa", "bb"});
  results.push_back({"a"});
  string input =
      "a\naa.bb\naaa.bbb.ccc\naaaa.bbbb.cccc.dddd\naaa.bbb.ccc\naa.bb\na";
  TestParser('.', input, results);
}

TEST(CSVTest, EscapedInput) {
  std::vector<std::vector<string>> results;
  string input;
  // Escaped comma in a field.
  input += R"(1",")"
           "\n";
  results.push_back({"1,"});
  // Multiple escaped commas.
  input += "2,\",,\",,\",,,\"\n";
  results.push_back({"2", ",,", "", ",,,"});
  // Empty and non-empty quoted fields.
  input += "3,\"\",\"3\"\n";
  results.push_back({"3", "", "3"});
  // Escaped quote sign.
  input += R"(4,"\"4\"",\"\")"
           "\n";
  results.push_back({"4", R"("4")", R"("")"});
  // Documentation example.
  input += R"(5,",",,"\"")";
  results.push_back({"5", ",", "", R"(")"});
  TestParser(',', input, results);
}

}  // unnamed namespace
}  // namespace util
}  // namespace tervuren
