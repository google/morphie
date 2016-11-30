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

// Initialize the Plaso analyzer with an empty JSON document.
#include <iostream>
#include <memory>
#include <sstream>

#include "analyzers/plaso/plaso_analyzer.h"
#include "json_reader.h"
#include "json/json.h"

int main(int argc, char **argv) {
  morphie::PlasoAnalyzer analyzer(false /* Do not show event sources.*/);
  std::istringstream stream("");
  morphie::StreamJson json_stream(&stream);
  analyzer.Initialize(&json_stream);
  std::cout << "Initialized Plaso analyzer." << std::endl;
}
