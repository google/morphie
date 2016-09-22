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

#include "graph_exporter.h"

#include <unordered_map>

#include "ast.h"
#include "util/string_utils.h"

namespace tervuren {
namespace viz {

GraphExporter::GraphExporter(const LabeledGraph& graph)
    : graph_(graph), node_label_(TextLabel) {}

GraphExporter::GraphExporter(const LabeledGraph& graph,
                             const LabelFn& node_label)
    : graph_(graph), node_label_(node_label) {}

// Returns a serialization of 'ast' with '/' as a separator and no bounding
// delimiters. The serialization is prefixed by "tag/".
string GraphExporter::TextLabel(const string& tag, const AST& ast) {
  string label = tag;
  label += "/";
  label += ast::ToString(
      ast, ast::PrintConfig("", "", "/", ast::PrintOption::kValue));
  return label;
}

string GraphExporter::HTMLLabel(const string& tag, const AST& ast) {
  ast::PrintConfig config;
  if (tag == "Event") {
    config =
        ast::PrintConfig("<div>", "<br>", "</div>", ast::PrintOption::kValue);
  } else {
    config = ast::PrintConfig("", "", "/", ast::PrintOption::kValue);
  }
  return ast::ToString(ast, config);
}

ge::GraphDef GraphExporter::Graph() {
  ge::GraphDef vis_graph;
  for (auto node_it = graph_.NodeSetBegin(); node_it != graph_.NodeSetEnd();
       ++node_it) {
    ge::Node* vis_node = vis_graph.add_node();
    *vis_node = Node(*node_it);
  }
  return vis_graph;
}

string GraphExporter::GraphAsString() { return Graph().DebugString(); }

string GraphExporter::NodeName(NodeId node_id, const string& tag,
                               const AST& ast) {
  string label = TextLabel(tag, ast);
  label += "/";
  label += std::to_string(node_id);
  return label;
}

string GraphExporter::FindOrAddName(NodeId node_id) {
  auto node_name_it = node_name_.find(node_id);
  if (node_name_it != node_name_.end()) {
    return node_name_it->second;
  } else {
    TaggedAST node_label = graph_.GetNodeLabel(node_id);
    string label = NodeName(node_id, node_label.tag(), node_label.ast());
    node_name_.emplace(node_id, label);
    return label;
  }
}

ge::Node GraphExporter::Node(NodeId node_id) {
  ge::Node vis_node;
  if (!graph_.HasNode(node_id)) {
    return vis_node;
  }
  // The node name is an identifier for the node.
  string node_name = FindOrAddName(node_id);
  vis_node.set_name(node_name);
  // The label is the string displayed on the node.
  TaggedAST label_ast = graph_.GetNodeLabel(node_id);
  string label_str = node_label_(label_ast.tag(), label_ast.ast());
  // vis_node.set_label(HTMLLabel(node_label.tag(), node_label.ast()));
  // Set node attributes.
  auto& node_attr = *vis_node.mutable_node_attr();
  node_attr["label"] = label_str;
  // The value of this field can be used to automatically color a metanode by
  // the frequency of types of nodes within the metanode.
  node_attr["op"] = "op";
  set<NodeId> in_nodes = graph_.GetPredecessors(node_id);
  string in_node_name;
  for (auto in_node : in_nodes) {
    in_node_name = FindOrAddName(in_node);
    TaggedAST in_node_label = graph_.GetNodeLabel(in_node);
    string in_node_name =
        NodeName(in_node, in_node_label.tag(), in_node_label.ast());
    ge::Edge* edge = vis_node.add_edge();
    edge->set_input(in_node_name);
  }
  return vis_node;
}

}  // namespace viz
}  // namespace tervuren
