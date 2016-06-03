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

// This file contains definitions of constants used for account access analysis.
#ifndef LOGLE_ACCOUNT_ACCESS_DEFS_H_
#define LOGLE_ACCOUNT_ACCESS_DEFS_H_

namespace logle {
// The 'access' namespace contains definitions of fields used in access-oriented
// analysis of access to email accounts.
namespace access {

// A comma-separated list of fields that must be present in the input.
extern const char kRequiredFields[];
// The strings below are the names of the fields that provide information about
// a single account access.
//
// The account used to perform an access.
extern const char kActor[];
// The manager of the accessor.
extern const char kActorManager[];
// The job title of the accessor (Software Engineer, Content Reviewer, etc.).
extern const char kActorTitle[];
// The number of accesses.
extern const char kNumAccesses[];
// The account that is accessed.
extern const char kUser[];

}  // namespace access
}  // logle

#endif  // LOGLE_ACCOUNT_ACCESS_DEFS_H_
