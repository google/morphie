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
#ifndef LOGLE_UTIL_STRING_UTILS_TEST_H_
#define LOGLE_UTIL_STRING_UTILS_TEST_H_

#include "util/string_utils.h"

#include <set>

#include "base/string.h"
#include "gtest.h"

namespace logle {
namespace util {
namespace {

TEST(VectorSplitTest, Correctness) {
  std::vector<string> result;
  // Empty string.
  result = {""};
  EXPECT_EQ(result, SplitToVector("", '.'));
  // String with no delimiters.
  result = {"abc"};
  EXPECT_EQ(result, SplitToVector("abc", '.'));
  // Delimiters only.
  result = {"", ""};
  EXPECT_EQ(result, SplitToVector(",", ','));
  result = {"", "a"};
  EXPECT_EQ(result, SplitToVector(",a", ','));
  result = {"", "a", ""};
  EXPECT_EQ(result, SplitToVector(",a,", ','));
  result = {"a", "b", "a"};
  EXPECT_EQ(result, SplitToVector("a/b/a", '/'));
  result = {"a", "b", "a", ""};
  EXPECT_EQ(result, SplitToVector("a/b/a/", '/'));
  result = {"", "a", "b", "a", ""};
  EXPECT_EQ(result, SplitToVector("/a/b/a/", '/'));
  // Empty string between non-empty input. The test string is:
  //   "a[space][space]b[space]c[space]"
  result = {"a", "", "b", "c", ""};
  EXPECT_EQ(result, SplitToVector("a  b c ", ' '));
}

TEST(SetSplitTest, Correctness) {
  std::set<string> result;
  // Empty string.
  result = {""};
  EXPECT_EQ(result, SplitToSet("", '.'));
  // Delimiters only.
  result = {""};
  EXPECT_EQ(result, SplitToSet(",,", ','));
  result = {"a", "b"};
  EXPECT_EQ(result, SplitToSet("a/b/a", '/'));
  // Empty string between non-empty input.
  result = {"a", "", "b"};
  EXPECT_EQ(result, SplitToSet("a  b a ", ' '));
}

TEST(StrCatTest, Correctness) {
  // Empty strings.
  EXPECT_EQ("", StrCat("", ""));
  EXPECT_EQ("", StrCat("", "", ""));
  EXPECT_EQ("", StrCat("", "", "", ""));
  EXPECT_EQ("", StrCat("", "", "", "", ""));
  // Combination of non-empty and empty strings.
  EXPECT_EQ("a", StrCat("a", ""));
  EXPECT_EQ("a", StrCat("", "a"));
  EXPECT_EQ("a", StrCat("", "a", ""));
  EXPECT_EQ("ab", StrCat("", "a", "b", ""));
  EXPECT_EQ("abc", StrCat("a", "", "b", "", "c"));
  // Non-empty strings.
  EXPECT_EQ("ab", StrCat("a", "b"));
  EXPECT_EQ("abc", StrCat("a", "b", "c"));
  EXPECT_EQ("abcd", StrCat("a", "b", "c", "d"));
  EXPECT_EQ("abcde", StrCat("a", "b", "c", "d", "e"));
  EXPECT_EQ("abcdef", StrCat("a", "b", "c", "d", "e", "f"));
}

TEST(StrAppendTest, Correctness) {
  string out;
  // Appending single characters.
  StrAppend(&out, "");
  EXPECT_EQ("", out);
  StrAppend(&out, "a");
  EXPECT_EQ("a", out);
  StrAppend(&out, "b");
  EXPECT_EQ("ab", out);
  // Exercise every variant of the StrAppend function.
  out.clear();
  StrAppend(&out, "a", "b");
  EXPECT_EQ("ab", out);
  out.clear();
  StrAppend(&out, "a", "b", "c");
  EXPECT_EQ("abc", out);
  out.clear();
  StrAppend(&out, "a", "b", "c", "d");
  EXPECT_EQ("abcd", out);
  out.clear();
  StrAppend(&out, "a", "b", "c", "d", "e");
  EXPECT_EQ("abcde", out);
  out.clear();
  StrAppend(&out, "a", "b", "c", "d", "e", "f");
  EXPECT_EQ("abcdef", out);
  // The next few tests demonstrate anti-patterns. Where the string being
  // appended to is passed as data as well.
  // This will work because 'out' occurs only once as an argument.
  out = "ab";
  StrAppend(&out, out);
  EXPECT_EQ("abab", out);
  // These do not work because the string is modified as it is being read for
  // each argument.
  out.clear();
  StrAppend(&out, out, "ab", out);
  EXPECT_NE("ab", out);
}

TEST(SetJoinTest, OutputProperties) {
  // Inputs that map to the empty string.
  // Sets that map to the empty string.
  // The empty set.
  EXPECT_EQ("", SetJoin(std::set<string>{}, ","));
  EXPECT_EQ("", SetJoin(std::set<int>{}, ","));
  // A set containing only the empty string.
  EXPECT_EQ("", SetJoin(std::set<string>{""}, ","));
  EXPECT_EQ("", SetJoin(std::set<string>{"", ""}, ""));
  EXPECT_EQ("", SetJoin(std::set<string>{"", ""}, ", "));
  // Inputs mapping to output with no delimiters.
  EXPECT_EQ("a", SetJoin(std::set<string>{"a"}, ","));
  EXPECT_EQ("a", SetJoin(std::set<string>{"a", "a"}, ","));
  EXPECT_EQ("aa", SetJoin(std::set<string>{"aa"}, ","));
  EXPECT_EQ("1", SetJoin(std::set<int>{1}, ","));
  // Inputs mapping to outputs with delimiters.
  EXPECT_EQ("a,b", SetJoin(std::set<string>{"a", "b"}, ","));
  EXPECT_EQ("a, b, c", SetJoin(std::set<string>{"a", "b", "c"}, ", "));
  EXPECT_EQ(", a", SetJoin(std::set<string>{"", "a"}, ", "));
  // std::set is ordered, so the empty string appears first in the next output.
  EXPECT_EQ(", a", SetJoin(std::set<string>{"a", ""}, ", "));
  EXPECT_EQ("1, 2", SetJoin(std::set<int>{1, 2}, ", "));
}

}  // anonymous namespace
}  // namespace util
}  // namespace logle

#endif  // LOGLE_UTIL_STRING_UTILS_TEST_H_
