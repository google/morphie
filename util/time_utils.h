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

// This file contains functions for converting between a civil time format and
// an absolute time representation. Civil time refers to a calendar date and
// clock time possibly augmented with timezone and daylight savings time
// information. Civil time is ambiguous because timezone and daylight savings
// information may be missing and because it is influenced by historical
// adjustments. Absolute time is the number of ticks of a clock of fixed
// duration since a uniquely chosen timepoint called an epoch. The functions in
// this file convert between absolute time measured as microseconds since the
// Unix epoch and civil time in RFC3339 format.
#ifndef LOGLE_UTIL_TIME_UTILS_H_
#define LOGLE_UTIL_TIME_UTILS_H_

#include <cstdint>

#include "base/string.h"

namespace tervuren {
namespace util {

// Returns an RFC3339 string in UTC corresponding to the number of microseconds
// since the Unix epoch.
string UnixMicrosToRFC3339(int64_t unix_micros);

// Returns true and stores the microseconds since the Unix epoch in *unix_micros
// if time_str could be parsed as an RFC3339 string. Returns false otherwise.
bool RFC3339ToUnixMicros(const string& time_str, int64_t* unix_micros);

}  // namespace util
}  // namespace tervuren

#endif  // LOGLE_UTIL_TIME_UTILS_H_
