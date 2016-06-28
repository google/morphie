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

// Logle is a tool for analyzing logs using graph algorithms and graph
// visualization techniques. It can run one of many different analyses on a log
// file in either CSV or JSON format.
//
// Development is at an early stage.
#include <iostream>

#include "base/commandlineflags.h"
#include "base/init_google.h"
#include "base/integral_types.h"
#include <google/protobuf/text_format.h>
#include "third_party/logle/analysis_options.pb.h"
#include "third_party/logle/base/string.h"
#include "third_party/logle/frontend.h"
#include "third_party/logle/util/status.h"

namespace logle = third_party_logle;
namespace protobuf = google::protobuf;
using Analyzer = third_party_logle::frontend::Analyzer;

// The input string containing analysis options as a human-readable protobuf.
DEFINE_string(analysis_options, "", "Analysis options as a protocol buffer.");

// Returns true and logs an error message if 'value' is empty. This function is
// called by InitGoogle. More complex validation takes place separately.
static bool CheckFlagValueNotEmpty(const char* flagname, const string& value) {
  if (value.empty()) {
    std::cerr << flagname << " flag must be non-empty.";
    return false;
  }
  return true;
}

static bool is_config_empty =
    RegisterFlagValidator(&FLAGS_analysis_options, &CheckFlagValueNotEmpty);

int main(int argc, char** argv) {
  InitGoogle(argv[0], &argc, &argv, true);
  logle::AnalysisOptions options;
  string out;
  if (!protobuf::TextFormat::ParseFromString(FLAGS_analysis_options,
                                             &options)) {
    std::cerr << "The 'analysis_options' flag must have the format of an "
                 "AnalysisOptions proto.";
    return -1;
  }
  logle::util::Status status = logle::frontend::Run(options);
  if (!status.ok()) {
    std::cerr << status.message();
    return -1;
  }
  return 0;
}
