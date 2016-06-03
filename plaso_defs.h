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

// This file contains definitions of constants specific to Plaso output.
#ifndef LOGLE_PLASO_DEFS_H_
#define LOGLE_PLASO_DEFS_H_

namespace logle {
namespace plaso {

// A comma-separated list of fields that must be present in the input.
extern const char kRequiredFields[];

// The strings below are names of fields associated with a specific event.
// A Plaso-internal canonical descriptor of the format of the event data.
extern const char kDataTypeName[];
// A human-readable description of the event.
extern const char kDescriptionName[];
// Details about an event.
extern const char kMessageName[];
// File from which the event was reconstructed by Plaso.
extern const char kSourceFileName[];
// An RFC3339 timestamp representing the time at which the event occurred.
extern const char kTimestampName[];

}  // namespace plaso
}  // logle

#endif  // LOGLE_PLASO_DEFS_H_
