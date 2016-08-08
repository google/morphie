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

// Initializes the account access analyzer and prints a message.
#include <iostream>
#include <memory>

#include "account_access_analyzer.h"

int main(int argc, char **argv) {
  tervuren::AccessAnalyzer analyzer;
  std::unique_ptr<tervuren::util::CSVParser> parser(
      new tervuren::util::CSVParser(new std::stringstream("")));
  analyzer.Initialize(std::move(parser));
  std::cout << "Initialized account access analyzer." << std::endl;
}
