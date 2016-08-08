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

// Utilities for manipulating strings.
#ifndef LOGLE_UTIL_STRING_UTILS_H_
#define LOGLE_UTIL_STRING_UTILS_H_

#include <set>
#include <sstream>
#include <vector>

#include "base/string.h"

namespace tervuren {
namespace util {

// Returns a container of strings obtained by splitting 'str' using the
// delimiter provided.
std::set<string> SplitToSet(const string& str, char delim);
// Some examples to illustrate the semantics.
//   SplitToVector("", ',') == {""}
//   SplitToVector("abc", ',') == {"abc"}
//   SplitToVector("/usr/local/bin", '/') == {"", "local", "bin"}
//   SplitToVector("/usr/local/bin/", '/') == {"", "local", "bin", ""}
std::vector<string> SplitToVector(const string& str, char delim);

// The StrCat functions return the concatenation of their arguments.
string StrCat(const string& str1, const string& str2);
string StrCat(const string& str1, const string& str2, const string& str3);
string StrCat(const string& str1, const string& str2, const string& str3,
              const string& str4);
string StrCat(const string& str1, const string& str2, const string& str3,
              const string& str4, const string& str5);
string StrCat(const string& str1, const string& str2, const string& str3,
              const string& str4, const string& str5, const string& str6);

// The StrAppend functions append the second and subsequent arguments to the
// string in the first argument. These functions should not be called with the
// output string as one of the second or other arguments. Please avoid
// anti-patterns like this.
//   StrAppend(&out, str1, out);
void StrAppend(string* out, const string& str1);
void StrAppend(string* out, const string& str1, const string& str2);
void StrAppend(string* out, const string& str1, const string& str2,
               const string& str3);
void StrAppend(string* out, const string& str1, const string& str2,
               const string& str3, const string& str4);
void StrAppend(string* out, const string& str1, const string& str2,
               const string& str3, const string& str4, const string& str5);
void StrAppend(string* out, const string& str1, const string& str2,
               const string& str3, const string& str4, const string& str5,
               const string& str6);

// Returns a string equivalent to the concatenation of strings in 'args' using
// 'sep' as a separator. Assumes that stream operators can be used to obtain a
// serialization of elements of type 'T'.
template <typename T>
string SetJoin(const std::set<T>& args, const string& sep) {
  if (args.empty()) {
    return "";
  }

  std::stringstream args_str;
  auto set_it = args.begin();
  args_str << *set_it;
  for (++set_it; set_it != args.end(); ++set_it) {
    args_str << sep << *set_it;
  }
  return args_str.str();
}

}  // namespace util
}  // namespace tervuren

#endif  // LOGLE_UTIL_STRING_UTILS_H_
