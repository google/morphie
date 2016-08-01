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

// Tests that the build can link to the external GFlags library (formerly Google
// Flags). The code below should generate an executable that does nothing but
// must be invoked with the flag "--test_flag" with a non-empty argument.
#include <iostream>
#include <string>

#include "gflags/gflags.h"

DEFINE_string(test_flag, "", "Value of the input flag.");

static bool CheckFlagValueNotEmpty(const char* flagname,
                                   const std::string& value) {
  if (value.empty()) {
    std::cerr << "The flat " << flagname << " must be non-empty.";
    return false;
  }
  return true;
}

static bool is_config_empty =
    google::RegisterFlagValidator(&FLAGS_test_flag, &CheckFlagValueNotEmpty);

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return 0;
}
