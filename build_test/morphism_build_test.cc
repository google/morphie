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

// Instantiate a morphism with an empty graph and print the result.
#include "morphism.h"

#include <iostream>

#include "ast.pb.h"
#include "labeled_graph.h"
#include "type.h"

int main(int argc, char **argv) {
  tervuren::LabeledGraph graph;
  tervuren::AST ast = tervuren::ast::type::MakeInt("int label", false);
  graph.Initialize({}, {}, {}, {}, ast);
  tervuren::graph::Morphism morphism(&graph);
  std::cout << "Initialized morphism." << std::endl;
}
