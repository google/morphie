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
#include "util/string_utils.h"

namespace morphie {
namespace util {

std::set<string> SplitToSet(const string& str, char delim) {
  std::vector<string> vec = SplitToVector(str, delim);
  return std::set<string>(vec.begin(), vec.end());
}

std::vector<string> SplitToVector(const string& str, char delim) {
  if (str.empty()) {
    return {""};
  }
  std::vector<string> strings;
  std::stringstream stream(str);
  string token;
  while (std::getline(stream, token, delim)) {
    strings.push_back(token);
  }
  if (str.back() == delim) {
    strings.push_back("");
  }
  return strings;
}

string StrCat(const string& str1, const string& str2) {
  return StrCat(str1, str2, "", "", "", "");
}

string StrCat(const string& str1, const string& str2, const string& str3) {
  return StrCat(str1, str2, str3, "", "", "");
}

string StrCat(const string& str1, const string& str2, const string& str3,
              const string& str4) {
  return StrCat(str1, str2, str3, str4, "", "");
}

string StrCat(const string& str1, const string& str2, const string& str3,
              const string& str4, const string& str5) {
  return StrCat(str1, str2, str3, str4, str5, "");
}

string StrCat(const string& str1, const string& str2, const string& str3,
              const string& str4, const string& str5, const string& str6) {
  string out;
  StrAppend(&out, str1, str2, str3, str4, str5, str6);
  return out;
}

void StrAppend(string* out, const string& str1) {
  StrAppend(out, str1, "", "", "", "", "");
}

void StrAppend(string* out, const string& str1, const string& str2) {
  StrAppend(out, str1, str2, "", "", "", "");
}

void StrAppend(string* out, const string& str1, const string& str2,
               const string& str3) {
  StrAppend(out, str1, str2, str3, "", "", "");
}

void StrAppend(string* out, const string& str1, const string& str2,
               const string& str3, const string& str4) {
  StrAppend(out, str1, str2, str3, str4, "", "");
}

void StrAppend(string* out, const string& str1, const string& str2,
               const string& str3, const string& str4, const string& str5) {
  StrAppend(out, str1, str2, str3, str4, str5, "");
}

void StrAppend(string* out, const string& str1, const string& str2,
               const string& str3, const string& str4, const string& str5,
               const string& str6) {
  out->reserve(out->size() + str1.size() + str2.size() + str3.size() +
               str4.size() + str5.size() + str6.size());
  *out += str1;
  *out += str2;
  *out += str3;
  *out += str4;
  *out += str5;
  *out += str6;
}

}  // namespace util
}  // namespace morphie
