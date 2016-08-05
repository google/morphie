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

// This file defines a function for checking assertions and printing an error
// if the assertion fails.
#ifndef THIRD_PARTY_LOGLE_UTIL_LOGGING_H_
#define THIRD_PARTY_LOGLE_UTIL_LOGGING_H_

#include "third_party/logle/base/string.h"

#define MAKE_STR(x) #x
#define TOSTRING(x) MAKE_STR(x)
#define LOCATION_STR __FILE__ ":" TOSTRING(__LINE__)
#define CHECK(c, err) util::Check(c, LOCATION_STR, err)
#define FAIL(err) util::Check(false, LOCATION_STR, err)

namespace tervuren {
namespace util {

// Produces an error message and aborts if the condition is false.
void Check(bool condition, const string& location, const string& err);
void Check(bool condition, const string& location);
void Check(bool condition);

}  // namespace util
}  // namespace tervuren

#endif  // THIRD_PARTY_LOGLE_UTIL_LOGGING_H_
