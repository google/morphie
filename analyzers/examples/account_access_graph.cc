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

#include "analyzers/examples/account_access_graph.h"

#include <cstdint>
#include <unordered_map>

#include "analyzers/examples/account_access_defs.h"
#include "graph/dot_printer.h"
#include "graph/type.h"
#include "graph/type_checker.h"
#include "util/logging.h"
#include "util/string_utils.h"
#include "graph/value.h"

namespace {
// These declarations are required because the using declaration in
// base/string.h and below are within the logle namespace.
using std::string;
using std::unordered_map;

// Error messages.
const char kInitializationErr[] = "The graph is not initialized.";
const char kNoTagErr[] = "The graph has no type tagged :";

// Tags and names for components of labels.
const char kActorTag[] = "Actor";
const char kAccessEdgeTag[] = "Access";
const char kAccessGraphTag[] = "Access Graph";
const char kCount[] = "Count";
const char kManager[] = "Manager";
const char kTitle[] = "Title";
const char kUserTag[] = "User";

// Returns the entry in 'fields' containing data for 'field_name'. The 'CHECK'
// statements here are the main input validation checks.
string GetField(const string& field_name,
                const unordered_map<string, int>& field_index,
                const std::vector<string>& fields) {
  const auto field_it = field_index.find(field_name);
  morphie::CHECK(
      field_it != field_index.end(),
      (morphie::util::StrCat("No field named ", field_name, " in input.")));
  morphie::CHECK(0 <= field_it->second,
               (morphie::util::StrCat("Index of ", field_name, " is negative.")));
  morphie::CHECK(
      static_cast<int>(fields.size()) > field_it->second,
      (morphie::util::StrCat("Index of ", field_name, " exceeds bounds.")));
  return fields[field_it->second];
}

}  // namespace

namespace morphie {

namespace type = ast::type;
namespace value = ast::value;

util::Status AccountAccessGraph::Initialize() {
  // Create a unique node label of type tuple(string, string, string) for actor
  // information.
  vector<AST> args;
  args.emplace_back(type::MakeString(kActorTag, false));
  args.emplace_back(type::MakeString(kTitle, true));
  args.emplace_back(type::MakeString(kManager, true));
  type::Types node_types;
  node_types.emplace(kActorTag, type::MakeTuple(kActorTag, false, args));
  // Create a unique node label of type string for user names.
  node_types.emplace(kUserTag, type::MakeString(kUserTag, false));
  set<string> unique_nodes = {kActorTag, kUserTag};
  // Define an access edge whose label is the number of accesses.
  type::Types edge_types;
  edge_types.emplace(kAccessEdgeTag, type::MakeInt(kCount, false));
  set<string> unique_edges = {kAccessEdgeTag};
  // There is no graph-level label.
  AST graph_type = type::MakeNull(kAccessGraphTag);

  util::Status s = graph_.Initialize(node_types, unique_nodes, edge_types,
                                     unique_edges, graph_type);
  if (s.ok()) {
    is_initialized_ = true;
    return s;
  }
  return util::Status(Code::INTERNAL, s.message());
}

int AccountAccessGraph::NumNodes() const {
  CHECK(is_initialized_, kInitializationErr);
  return graph_.NumNodes();
}

int AccountAccessGraph::NumLabeledNodes(const TaggedAST& label) const {
  CHECK(is_initialized_, kInitializationErr);
  return graph_.NumLabeledNodes(label);
}

int AccountAccessGraph::NumEdges() const {
  CHECK(is_initialized_, kInitializationErr);
  return graph_.NumEdges();
}

int AccountAccessGraph::NumLabeledEdges(const TaggedAST& label) const {
  CHECK(is_initialized_, kInitializationErr);
  return graph_.NumLabeledEdges(label);
}

void AccountAccessGraph::ProcessAccessData(
    const unordered_map<string, int>& field_index,
    const vector<string>& fields) {
  CHECK(is_initialized_, kInitializationErr);
  CHECK(!field_index.empty(), "The map 'field_index' is empty");
  CHECK(!fields.empty(), "The vector 'fields' is empty");
  TaggedAST actor = MakeActorLabel(field_index, fields);
  NodeId actor_id = graph_.FindOrAddNode(actor);
  TaggedAST user = MakeUserLabel(field_index, fields);
  NodeId user_id = graph_.FindOrAddNode(user);
  TaggedAST count = MakeEdgeLabel(field_index, fields);
  graph_.FindOrAddEdge(actor_id, user_id, count);
}

string AccountAccessGraph::ToDot() const {
  CHECK(is_initialized_, kInitializationErr);
  return DotPrinter().DotGraph(graph_);
}

TaggedAST AccountAccessGraph::MakeActorLabel(
    const unordered_map<string, int>& field_index,
    const vector<string>& fields) {
  // Create a tuple consisting of the actor, title and manager.
  AST actor_ast = value::MakeNullTuple(3);
  std::pair<bool, AST> result = graph_.GetNodeType(kActorTag);
  CHECK(result.first, (util::StrCat(kNoTagErr, kActorTag)));
  string field = GetField(access::kActor, field_index, fields);
  value::SetField(result.second, 0, value::MakeString(field), &actor_ast);
  field = GetField(access::kActorTitle, field_index, fields);
  value::SetField(result.second, 1, value::MakeString(field), &actor_ast);
  field = GetField(access::kActorManager, field_index, fields);
  value::SetField(result.second, 2, value::MakeString(field), &actor_ast);
  // Add a tag to the tuple.
  TaggedAST actor;
  *actor.mutable_ast() = actor_ast;
  actor.set_tag(kActorTag);
  return actor;
}

TaggedAST AccountAccessGraph::MakeUserLabel(
    const unordered_map<string, int>& field_index,
    const vector<string>& fields) {
  string user_str = GetField(access::kUser, field_index, fields);
  AST user_ast = value::MakeString(user_str);
  TaggedAST user;
  *user.mutable_ast() = user_ast;
  user.set_tag(kUserTag);
  return user;
}

TaggedAST AccountAccessGraph::MakeEdgeLabel(
    const unordered_map<string, int>& field_index,
    const vector<string>& fields) {
  string count_str = GetField(access::kNumAccesses, field_index, fields);
  // Convert a string to a 64 bit signed integer.
  int64_t count_val = std::stoll(count_str);
  TaggedAST count;
  *count.mutable_ast() = value::MakeInt(count_val);
  count.set_tag(kAccessEdgeTag);
  return count;
}

}  // namespace morphie
