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

// There are different types for representing time in C++.
//  - std::time_t is a value type supporting arithmetic that represents a
//  duration since the start of an unspecified epoch. This type is inherited
//  from the C time libraries. The unit for time_t is unspecified in the
//  standard. It often represents seconds since the start of the Unix epoch.
//  - std::tm is a struct, also from the C time library, for representing civil
//  time. It contains fields for the year, month, day, hour, minute, second and
//  daylight savings time but does not have a timezone field.
//  - std::chrono:time_point is a C++11 type with explicit representation of the
//    underlying clock and duration.
// The C++11 standard defines the functions std::put_time and std::get_time,
// which allow for parsing and printing civil time and take into account
// timezones and daylight savings time. However, these functions were only
// implemented in GCC 5. To avoid requiring that this code be compiled with GCC
// versions 5 and above, C time function are used to parse and print time.
// These functions ignore timezone information when parsing and use an
// inaccessible value for the local timezone when converting civil time to
// absolute time. The code below attempts to infer the implicit timezone and
// daylight savings information when possible.
#include "third_party/logle/util/time_utils.h"

#include <cstdio>
#include <ctime>

namespace tervuren {
namespace util {

// Uses the function gmtime_r to convert time in seconds to civil time in UTC
// and strftime to print this time.
string UnixMicrosToRFC3339(int64_t unix_micros) {
  std::tm tm;
  std::time_t unix_secs = unix_micros / 1000000;
  if (unix_micros % 1000000 < 0) {
    unix_secs--;
  }
  std::tm* tmp = gmtime_r(&unix_secs, &tm);
  char buf[sizeof("9223372036854775807-12-31T23:59:59+00:00")];
  std::strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S+00:00", tmp);
  return buf;
}

// Uses strptime to parse time_str and populate the fields in tm. Explicitly
// extracts the timezone offset, which is not parsed by strptime.
bool RFC3339ToUnixMicros(const string& time_str, int64_t* unix_micros) {
  std::tm tm;
  char* ap = const_cast<char*>(time_str.c_str());
  char* bp = strptime(ap, "%Y-%m-%dT%H:%M:%S", &tm);
  if (bp == nullptr) {
    return false;
  }
  if (*bp != '+' && *bp != '-') {
    return false;
  }
  int hour = 0;
  int minute = 0;
  int pos = 0;
  int nitems = sscanf(bp, "%d:%d%n", &hour, &minute, &pos);
  if (nitems != 2 && nitems != 3) {
    return false;
  }
  if (pos != 6 || bp[pos] != '\0') {
    return false;
  }
  int sign = -1;
  if (hour < 0) {
    hour = -hour;
    sign = 1;
  }
  if (hour > 23 || minute < 0 || minute > 59) {
    return false;
  }
  // Convert civil time to absolute time.
  int64_t seconds = tm.tm_sec;
  seconds += tm.tm_min * 60;
  seconds += tm.tm_hour * 3600;
  seconds += tm.tm_yday * 86400;
  seconds += (tm.tm_year - 70) * 31536000;
  seconds += ((tm.tm_year - 69) / 4) * 86400;
  seconds += sign * (((hour * 60) + minute) * 60);
  *unix_micros = seconds * 1000000;
  return true;
}

}  // namespace util
}  // namespace tervuren
