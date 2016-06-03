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

#include "plaso_defs.h"

namespace logle {
namespace plaso {

const char kRequiredFields[] =
    "data_type,datetime,display_name,message,timestamp_desc";

const char kDataTypeName[] = "data_type";
const char kDescriptionName[] = "timestamp_desc";
const char kMessageName[] = "message";
const char kSourceFileName[] = "display_name";
const char kTimestampName[] = "datetime";

}  // namespace plaso
}  // namespace logle
