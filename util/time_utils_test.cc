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

#include "util/time_utils.h"

#include <cstdint>

#include "base/string.h"
#include "gtest.h"

namespace morphie {
namespace util {
namespace {

TEST(TimeUtilsTest, RFC3339ToMicros) {
  // One microsecond after the epoch is still part of the 0-th second, but one
  // microsecond before the epoch is part of the previous second.
  EXPECT_EQ("1970-01-01T00:00:00+00:00", UnixMicrosToRFC3339(0));
  EXPECT_EQ("1970-01-01T00:00:00+00:00", UnixMicrosToRFC3339(1));
  EXPECT_EQ("1970-01-01T00:00:01+00:00", UnixMicrosToRFC3339(1000000));
  EXPECT_EQ("1969-12-31T23:59:59+00:00", UnixMicrosToRFC3339(-1));
  EXPECT_EQ("1969-12-31T23:59:59+00:00", UnixMicrosToRFC3339(-999999));
  EXPECT_EQ("2016-02-10T23:56:41+00:00", UnixMicrosToRFC3339(1455148601000000));
}

TEST(TimeUtilsTest, MicrosToRFC3339) {
  // Incomplete civil times or non-time strings cannot be parsed.
  int64_t micros;
  EXPECT_FALSE(RFC3339ToUnixMicros("", &micros));
  EXPECT_FALSE(RFC3339ToUnixMicros("foo", &micros));
  EXPECT_FALSE(RFC3339ToUnixMicros("1970-01-01", &micros));
  EXPECT_FALSE(RFC3339ToUnixMicros("1970-01-01T01:23", &micros));
  EXPECT_FALSE(RFC3339ToUnixMicros("1970-01-01T00:00:00+24:00", &micros));
  EXPECT_FALSE(RFC3339ToUnixMicros("1970-01-01T00:00:00-24:00", &micros));
  EXPECT_FALSE(RFC3339ToUnixMicros("1970-01-01T00:00:00+00:60", &micros));
  EXPECT_FALSE(RFC3339ToUnixMicros("1970-01-01T00:00:00+00:-2", &micros));
  // The start of the Unix epoch is absolute time 0.
  EXPECT_TRUE(RFC3339ToUnixMicros("1970-01-01T00:00:00+00:00", &micros));
  EXPECT_EQ(0, micros);
  EXPECT_TRUE(RFC3339ToUnixMicros("1970-01-01T00:00:01+00:00", &micros));
  EXPECT_EQ(1000000, micros);
  EXPECT_TRUE(RFC3339ToUnixMicros("1969-12-31T23:59:59+00:00", &micros));
  EXPECT_EQ(-1000000, micros);
  // 5:00PM is 7 hours before midnight, so the date below should be the
  // corresponding number of microseconds before absolute time 0.
  int64_t kSecsBeforeMidnight = 7 * 60 * 60;
  EXPECT_TRUE(RFC3339ToUnixMicros("1969-12-31T17:00:00+00:00", &micros));
  EXPECT_EQ(-1000000 * kSecsBeforeMidnight, micros);
}

}  // anonymous namespace
}  // namespace util
}  // namespace morphie
