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

// The code below runs the type checker on an AST representing a type.
#include <iostream>

#include "third_party/logle/graph/ast.pb.h"
#include "type.h"
#include "type_checker.h"

int main(int argc, char **argv) {
  morphie::AST ast = morphie::ast::type::MakeBool("ast", false);
  std::cout << ast.DebugString();
  std::string err;
  if (morphie::ast::type::IsType(ast, &err)) {
    std::cout << " is a type." << std::endl;
  } else {
    std::cout << " is not a type." << std::endl;
  }
}
