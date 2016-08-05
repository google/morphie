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

// This file contains functions that initialize and manipulate an event graph.
// Much of the work done in these functions is creating abstract syntax trees
// (ASTs) representing either types for labels or values for labels.
#include "third_party/logle/plaso_event_graph.h"

#include <boost/algorithm/string/join.hpp>  // NOLINT
#include <sstream>
#include <utility>

#include "third_party/logle/ast.h"
#include "third_party/logle/dot_printer.h"
#include "third_party/logle/plaso_event.h"
#include "third_party/logle/plaso_event.pb.h"
#include "third_party/logle/type.h"
#include "third_party/logle/type_checker.h"
#include "third_party/logle/util/logging.h"
#include "third_party/logle/util/string_utils.h"
#include "third_party/logle/util/time_utils.h"
#include "third_party/logle/value.h"

namespace tervuren {

namespace type = ast::type;
namespace value = ast::value;

namespace {

const char kInitializationErr[] = "The graph is not initialized.";
const char kGraphNodeErr[] = "Error adding node to graph.";
const char kGraphTypeErr[] = "Could not retrieve type information.";
const char kTemporalEdgesErr[] = "AddTemporalEdges() can be called at most "
    "once. Events cannot be added after it is called.";

// Tags for data annotating nodes.
const char kDescTag[] = "Description";
const char kEventTag[] = "Event";
const char kSystemTag[] = "System";

// A timeline for the Dot output is a vertical line annotated with timestamps in
// order with the earliest timestamp at the top.  Events are displayed at the
// same horizontal level as their timestamp in the timeline.
string GetTimeline(const map<int64_t, set<NodeId>>& time_index) {
  string timeline = "// Sub-graph showing timeline\n{\n";
  vector<string> timestamps;
  string node_name;
  string time_aligned_nodes;
  for (const auto& timed_events : time_index) {
    node_name = util::StrCat("T", std::to_string(timed_events.first));
    util::StrAppend(&timeline, "  ", node_name, " [shape=plaintext, ",
                    R"(label=")", util::UnixMicrosToRFC3339(timed_events.first),
                    "\"];\n");
    timestamps.emplace_back(node_name);
    util::StrAppend(&time_aligned_nodes, "  {rank=same; ", node_name, "; ",
                    util::SetJoin(timed_events.second, "; "), "}\n");
  }
  util::StrAppend(&timeline, "  ", boost::algorithm::join(timestamps, " -> "),
                  ";\n");
  util::StrAppend(&timeline, time_aligned_nodes);
  util::StrAppend(&timeline, "}  // subgraph for timeline \n");
  return timeline;
}

}  // namespace

util::Status PlasoEventGraph::Initialize() {
  // Create a non-unique node label of type tuple(timestamp, string) for events.
  vector<AST> args;
  args.emplace_back(type::MakeTimestamp(ast::kTimeTag, true));
  args.emplace_back(type::MakeString(kDescTag, true));
  type::Types node_types;
  node_types.emplace(kEventTag, type::MakeTuple(kEventTag, false, args));
  // Create a unique node label of type list(string) for files.
  node_types.emplace(ast::kFileTag, type::MakeFile());
  set<string> unique_nodes = {ast::kFileTag};
  // Create a unique node label of type string for IP Addresses.
  node_types.emplace(ast::kIPAddressTag, type::MakeIPAddress());
  unique_nodes.insert(ast::kIPAddressTag);
  // Create a unique node label of type string for URLs.
  node_types.emplace(ast::kURLTag, type::MakeURL());
  unique_nodes.insert(ast::kURLTag);
  // Create an unlabelled edge.
  type::Types edge_types;
  edge_types.emplace(ast::kPrecedesTag, type::MakeNull(ast::kPrecedesTag));
  edge_types.emplace(ast::kUsesTag, type::MakeNull(ast::kUsesTag));
  set<string> unique_edges = {ast::kPrecedesTag, ast::kUsesTag};
  // The graph is labelled by a string.
  AST graph_type = type::MakeString(kSystemTag, false);
  // Initialize graph_ with the node types above but no edge label types.
  util::Status s = graph_.Initialize(node_types, unique_nodes, edge_types,
                                     unique_edges, graph_type);
  if (s.ok()) {
    is_initialized_ = true;
    return s;
  }
  return util::Status(Code::INTERNAL, s.message());
}

int PlasoEventGraph::NumNodes() const {
  CHECK(is_initialized_, kInitializationErr);
  return graph_.NumNodes();
}

int PlasoEventGraph::NumLabeledNodes(const TaggedAST& label) const {
  CHECK(is_initialized_, kInitializationErr);
  return graph_.NumLabeledNodes(label);
}

int PlasoEventGraph::NumEdges() const {
  CHECK(is_initialized_, kInitializationErr);
  return graph_.NumEdges();
}

int PlasoEventGraph::NumLabeledEdges(const TaggedAST& label) const {
  CHECK(is_initialized_, kInitializationErr);
  return graph_.NumLabeledEdges(label);
}

string PlasoEventGraph::GetStats() const {
  return util::StrCat("Number of Nodes : ", std::to_string(NumNodes()), "\n",
                      "Number of Edges : ", std::to_string(NumEdges()), "\n");
}

void PlasoEventGraph::ProcessEvent(const PlasoEvent& event_data) {
  CHECK(is_initialized_, kInitializationErr);
  // See the documentation of AddTemporalEdges() in this file for the reason
  // behind the check for temporal edges.
  CHECK(!has_temporal_edges_, kTemporalEdgesErr);
  AST timestamp =
      (event_data.has_timestamp())
          ? value::MakeTimestampFromUnixMicros(event_data.timestamp())
          : value::MakeTimestampFromRFC3339("");
  AST source = (event_data.has_desc()) ? value::MakeString(event_data.desc())
                                       : value::MakeString("");
  NodeId event_id = graph_.FindOrAddNode(MakeEventLabel(timestamp, source));
  if (event_data.has_timestamp()) {
    time_index_[event_data.timestamp()].insert(event_id);
  }
  CHECK(event_id >= 0, "");
  AddEventData(event_id, event_data);
}

// A PlasoEventGraph uses edges to represent temporal relationships. This allows
// questions about the relationship between events in time and time-based
// manipulation to be reduced to graph algorithms. For instance, finding all
// events between two timestamps can be implemented using breadth first search.
// To prevent the number of edges in the graph from exploding, only edges
// between events that immediately follow each other (meaning there are no other
// events in between) are added to the graph. To maintain this property,
// temporal edges must only added after all events have been added to the graph.
// This example illustrates a problem that arises otherwise.
//
// Example. Consider four events: e1 occurs first, followed by e2 and e3 at the
//   same time, and e4 occurs last.  If AddTemporalEdges() is called multiple
//   times, there will be more edges in the graph than required.
//
//   ProcessEvent(field_index, fields1); // Adds e1 to the graph.
//   ProcessEvent(field_index, fields4); // Adds e4 to the graph.
//   AddTemporalEdges(); // Adds the edge (e1, e4)
//   ProcessEvent(field_index, fields2); // Adds e2 to the graph.
//   ProcessEvent(field_index, fields3); // Adds e3 to the graph.
//   AddTemporalEdges(); // Adds (e1, e2), (e1, e3), (e2, e4) and (e3, e4).
//
//   This graph now contains the edge (e1, e4), which is unnecessary. If
//   AddTemporalEdges can be called multiple times, edges would have to be
//   deleted from the graph and the implementation would be more complicated.
void PlasoEventGraph::AddTemporalEdges() {
  CHECK(is_initialized_, kInitializationErr);
  CHECK(!has_temporal_edges_, kTemporalEdgesErr);
  // There must be at least two different timestamps to add temporal edges,
  // hence the check below.
  if (time_index_.size() < 2) {
    return;
  }
  auto current_time_it = time_index_.begin();
  auto next_time_it = time_index_.begin();
  ++next_time_it;
  TaggedAST edge_label;
  edge_label.set_tag(ast::kPrecedesTag);
  *edge_label.mutable_ast() = value::MakeNull();
  while (next_time_it != time_index_.end()) {
    for (NodeId current_node : current_time_it->second) {
      for (NodeId next_node : next_time_it->second) {
        graph_.FindOrAddEdge(current_node, next_node, edge_label);
      }
    }
    ++current_time_it;
    ++next_time_it;
  }
}

void PlasoEventGraph::AddFile(NodeId node_id, const File& file,
                              bool is_source) {
  // Create a node for the file.
  TaggedAST label;
  label.set_tag(ast::kFileTag);
  *label.mutable_ast() = plaso::ToAST(file);
  NodeId file_id = graph_.FindOrAddNode(label);
  // Create an edge between the event and the file.
  TaggedAST edge_label;
  edge_label.set_tag(ast::kUsesTag);
  *edge_label.mutable_ast() = value::MakeNull();
  if (is_source) {
    graph_.FindOrAddEdge(file_id, node_id, edge_label);
  } else {
    graph_.FindOrAddEdge(node_id, file_id, edge_label);
  }
}

void PlasoEventGraph::AddResource(NodeId node_id, const string& tag,
                                  const string& resource, bool is_source) {
  // Create a node for the resource.
  TaggedAST label;
  label.set_tag(tag);
  *label.mutable_ast() = value::MakeString(resource);
  NodeId resource_id = graph_.FindOrAddNode(label);
  // Create an edge between the event and the file.
  TaggedAST edge_label;
  edge_label.set_tag(ast::kUsesTag);
  *edge_label.mutable_ast() = value::MakeNull();
  if (is_source) {
    graph_.FindOrAddEdge(node_id, resource_id, edge_label);
  } else {
    graph_.FindOrAddEdge(resource_id, node_id, edge_label);
  }
}

void PlasoEventGraph::AddEventData(NodeId node_id,
                                   const PlasoEvent& event_data) {
  if (event_data.has_source_file()) {
    AddFile(node_id, event_data.source_file(), true /*The file is a source.*/);
  }
  if (event_data.has_target_file()) {
    AddFile(node_id, event_data.target_file(), false /*The file is a target.*/);
  }
  if (event_data.has_source_url()) {
    AddResource(node_id, ast::kURLTag, event_data.source_url(),
                true /*The URL is a source.*/);
  }
  if (event_data.has_target_url()) {
    AddResource(node_id, ast::kURLTag, event_data.target_url(),
                true /*The URL is a target.*/);
  }
}

TaggedAST PlasoEventGraph::MakeEventLabel(const AST& timestamp,
                                          const AST& source) {
  AST event_ast = value::MakeNullTuple(2);
  std::pair<bool, AST> result = graph_.GetNodeType(kEventTag);
  CHECK(result.first, kGraphTypeErr);
  value::SetField(result.second, 0, timestamp, &event_ast);
  value::SetField(result.second, 1, source, &event_ast);
  TaggedAST event;
  *event.mutable_ast() = event_ast;
  event.set_tag(kEventTag);
  return event;
}


string PlasoEventGraph::ToDot() const {
  CHECK(is_initialized_, kInitializationErr);
  DotPrinter dot_printer;
  string dot_graph = dot_printer.AllNodesInDot(graph_);
  util::StrAppend(&dot_graph, GetTimeline(time_index_), "\n");
  util::StrAppend(&dot_graph, dot_printer.AllEdgesInDot(graph_), "\n");
  return util::StrCat("digraph logle_graph {\n", dot_graph, "}");
}

}  // namespace tervuren
