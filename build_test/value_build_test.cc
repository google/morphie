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

// The code below runs the value checker on an AST representing a value.
#include <iostream>
#include <string>

#include "third_party/logle/ast.pb.h"
#include "value.h"
#include "value_checker.h"

int main(int argc, char **argv) {
  morphie::AST ast = morphie::ast::value::MakeBool(false);
  std::string err;
  std::cout << ast.DebugString();
  if (morphie::ast::value::IsValue(ast, &err)) {
    std::cout << " is a value." << std::endl;
  } else {
    std::cout << " is not a value." << std::endl;
  }
}
