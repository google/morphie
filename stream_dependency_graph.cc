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

#include "stream_dependency_graph.h"

#include <set>
#include <utility>
#include "base/vector.h"

#include "ast.pb.h"
#include "curio_defs.h"
#include "dot_printer.h"
#include "type.h"
#include "type_checker.h"
#include "util/logging.h"
#include "util/string_utils.h"
#include "value.h"

namespace tervuren {

namespace type = ast::type;
namespace value = ast::value;

namespace {

const char kInitializationErr[] = "The graph is not initialized.";

// Returns a tagged abstract syntax tree (AST) in which the tag is
// curio::kStreamTag, and the AST represents the tuple of strings
//   (node_id, node_name).
// The AST is created using the factory function MakeNullTuple from value.h. The
// fields of the tuple are set using SetField. The argument 'type' must be a
// pair of strings because SetField guarantees that the field update respects
// the type of the tuple.
TaggedAST MakeNodeLabel(const AST& type, const string& node_id,
                        const string& node_name) {
  TaggedAST label;
  label.set_tag(curio::kStreamTag);
  AST* ast = label.mutable_ast();
  *ast = value::MakeNullTuple(2);
  value::SetField(type, 0, value::MakeString(node_id), ast);
  value::SetField(type, 1, value::MakeString(node_name), ast);
  return label;
}

}  // namespace

util::Status StreamDependencyGraph::Initialize() {
  // Create a unique node label of type tuple(string, string) for each stream.
  vector<AST> args;
  args.emplace_back(
      type::MakeString(curio::kStreamIdTag, false /*May not be null*/));
  args.emplace_back(
      type::MakeString(curio::kStreamNameTag, true /*May be null*/));
  type::Types node_types;
  node_types.emplace(
      curio::kStreamTag,
      type::MakeTuple(curio::kStreamTag, false /*May not be null*/, args));
  set<string> unique_nodes = {curio::kStreamTag};
  type::Types edge_types;
  edge_types.emplace(curio::kDependentTag,
                     type::MakeNull(curio::kDependentTag));
  set<string> unique_edges = {curio::kDependentTag};
  // There is no graph-level label.
  AST graph_type = type::MakeNull(curio::kStreamTag);

  util::Status s = graph_.Initialize(node_types, unique_nodes, edge_types,
                                     unique_edges, graph_type);
  if (!s.ok()) {
    return util::Status(Code::INTERNAL, s.message());
  }
  is_initialized_ = true;
  return s;
}

int StreamDependencyGraph::NumNodes() const {
  CHECK(is_initialized_, kInitializationErr);
  return graph_.NumNodes();
}

int StreamDependencyGraph::NumEdges() const {
  CHECK(is_initialized_, kInitializationErr);
  return graph_.NumEdges();
}

void StreamDependencyGraph::AddDependency(const string& consumer_id,
                                          const string& consumer_name,
                                          const string& producer_id,
                                          const string& producer_name) {
  CHECK(is_initialized_, kInitializationErr);
  std::pair<bool, AST> type = graph_.GetNodeType(curio::kStreamTag);
  TaggedAST consumer = MakeNodeLabel(type.second, consumer_id, consumer_name);
  TaggedAST producer = MakeNodeLabel(type.second, producer_id, producer_name);
  NodeId consumer_node = graph_.FindOrAddNode(consumer);
  NodeId producer_node = graph_.FindOrAddNode(producer);
  TaggedAST edge_label;
  edge_label.set_tag(curio::kDependentTag);
  *edge_label.mutable_ast() = value::MakeNull();
  graph_.FindOrAddEdge(consumer_node, producer_node, edge_label);
}

string StreamDependencyGraph::ToDot() const {
  CHECK(is_initialized_, kInitializationErr);
  AttributeFn node_attribute = [](const string& tag, const AST& ast) {
    return DotPrinter::NodeAttribute(tag, ast.c_ast().arg(1));
  };
  AttributeFn edge_attribute = DotPrinter::EdgeAttribute;

  DotPrinter dot_printer(node_attribute, edge_attribute);
  string dot_graph = dot_printer.AllNodesInDot(graph_);
  util::StrAppend(&dot_graph, dot_printer.AllEdgesInDot(graph_), "\n");
  return util::StrCat("digraph stream_dependencies {\n", dot_graph, "}");
}

}  // namespace tervuren
