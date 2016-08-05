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

#include "tf_graph_exporter.h"

#include <unordered_map>

#include "ast.h"
#include "util/string_utils.h"

namespace tervuren {

TFGraphExporter::TFGraphExporter(const LabeledGraph& graph)
    : graph_(graph), node_label_(NodeLabel) {}

TFGraphExporter::TFGraphExporter(const LabeledGraph& graph,
                                 const TFLabelFn& node_label)
    : graph_(graph), node_label_(node_label) {}

// Returns a serialization of 'ast' with '/' as a separator and no bounding
// delimiters. The serialization is prefixed by "tag/".
string TFGraphExporter::NodeLabel(const string& tag, const AST& ast) {
  string label = tag;
  label += "/";
  label += ast::ToString(
      ast, ast::PrintConfig("", "", "/", ast::PrintOption::kValue));
  return label;
}

string TFGraphExporter::NodeLabelWithId(NodeId node_id, const string& tag,
                                        const AST& ast) {
  string label = NodeLabel(tag, ast);
  label += "/";
  label += std::to_string(node_id);
  return label;
}

tensorflow::GraphDef TFGraphExporter::TFGraph() {
  tensorflow::GraphDef tf_graph;
  for (auto node_it = graph_.NodeSetBegin(); node_it != graph_.NodeSetEnd();
       ++node_it) {
    tensorflow::NodeDef* tf_node = tf_graph.add_node();
    *tf_node = TFNode(*node_it);
  }
  return tf_graph;
}

string TFGraphExporter::TFGraphAsString() { return TFGraph().DebugString(); }

string TFGraphExporter::FindOrAddName(NodeId node_id) {
  auto node_name_it = node_name_.find(node_id);
  if (node_name_it != node_name_.end()) {
    return node_name_it->second;
  } else {
    TaggedAST node_label = graph_.GetNodeLabel(node_id);
    string label = NodeLabelWithId(node_id, node_label.tag(), node_label.ast());
    node_name_.emplace(node_id, label);
    return label;
  }
}

tensorflow::NodeDef TFGraphExporter::TFNode(NodeId node_id) {
  tensorflow::NodeDef tf_node;
  if (!graph_.HasNode(node_id)) {
    return tf_node;
  }
  string node_name = FindOrAddName(node_id);
  tf_node.set_name(node_name);
  set<NodeId> in_nodes = graph_.GetPredecessors(node_id);
  string in_node_name;
  for (auto in_node : in_nodes) {
    in_node_name = FindOrAddName(in_node);
    TaggedAST in_node_label = graph_.GetNodeLabel(in_node);
    string in_node_name =
        NodeLabelWithId(in_node, in_node_label.tag(), in_node_label.ast());
    tf_node.add_input(in_node_name);
  }
  return tf_node;
}

}  // namespace tervuren
