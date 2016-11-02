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
#include "util/status.h"

#include "gtest.h"

namespace morphie {
namespace util {
namespace {

TEST(StatusTest, MemberFunctions) {
  Status s;
  // The default status object has the OK error code and empty message.
  EXPECT_TRUE(s.ok());
  EXPECT_EQ(Code::OK, s.code());
  EXPECT_EQ("", s.message());
  // Status::OK has the same properties as the default status object.
  EXPECT_TRUE(Status::OK.ok());
  EXPECT_EQ(Code::OK, Status::OK.code());
  EXPECT_EQ("", Status::OK.message());
  // Construct an error object with code different from OK.
  s = Status(Code::INTERNAL, "Internal error.");
  EXPECT_FALSE(s.ok());
  EXPECT_EQ(Code::INTERNAL, s.code());
  EXPECT_EQ("Internal error.", s.message());
}

}  // unnamed namespace
}  // namespace util
}  // namespace morphie
