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

#include "third_party/logle/tf_graph_exporter.h"

#include <iostream>

#include "third_party/logle/ast.h"
#include "third_party/logle/gtest.h"
#include "third_party/logle/labeled_graph.h"
#include "third_party/logle/type.h"
#include "third_party/logle/type_checker.h"
#include "third_party/logle/value.h"

namespace tervuren {
namespace {

const char kNodeLabelTag_[] = "NodeLabel";
const char kEdgeLabelTag_[] = "EdgeLabel";

// Create a graph with all the supported tag types and one unsupported tag type.
util::Status Initialize(LabeledGraph* graph) {
  ast::type::Types node_types;
  node_types.emplace(ast::kFileTag, ast::type::MakeFile());
  node_types.emplace(ast::kIPAddressTag, ast::type::MakeIPAddress());
  node_types.emplace(ast::kURLTag, ast::type::MakeURL());
  node_types.emplace(kNodeLabelTag_,
                     ast::type::MakeString(kNodeLabelTag_, false));
  set<string> node_tags = {ast::kFileTag, ast::kIPAddressTag, ast::kURLTag};
  ast::type::Types edge_types;
  edge_types.emplace(ast::kPrecedesTag, ast::type::MakeNull(ast::kPrecedesTag));
  edge_types.emplace(kEdgeLabelTag_,
                     ast::type::MakeString(kEdgeLabelTag_, false));
  set<string> edge_tags = {ast::kPrecedesTag};
  AST graph_type = ast::type::MakeString("Graph", false);
  return graph->Initialize(node_types, node_tags, edge_types, edge_tags,
                           graph_type);
}

class TFGraphExporterTest : public ::testing::Test {
 protected:
  NodeId AddNode(const string& tag, AST ast) {
    tast_.set_tag(tag);
    *tast_.mutable_ast() = ast;
    return graph_.FindOrAddNode(tast_);
  }

  void AddEdge(NodeId source, NodeId target, const string& tag, AST ast) {
    tast_.set_tag(tag);
    *tast_.mutable_ast() = ast;
    graph_.FindOrAddEdge(source, target, tast_);
  }

  AST label_ast_;
  TaggedAST tast_;
  LabeledGraph graph_;
};

TEST_F(TFGraphExporterTest, NodeLabels) {
  string tag = "foo";
  string label;
  // A string AST becomes the label "tag/string".
  label_ast_ = ast::value::MakeString("bar");
  label = TFGraphExporter::NodeLabel(tag, label_ast_);
  EXPECT_EQ("foo/bar", label);
  // An interval [3,5] becomes the label "tag/3/5".
  label_ast_ = ast::value::MakeIntInterval(3, 5);
  label = TFGraphExporter::NodeLabel(tag, label_ast_);
  EXPECT_EQ("foo/3/5", label);
  // A list [a,b,c] becomes the label "tag/a/b/c".
  // This type is a list of strings.
  AST list_type = ast::type::MakeList(
      kNodeLabelTag_, true, ast::type::MakeString(kNodeLabelTag_, false));
  label_ast_ = ast::value::MakeEmptyList();
  AST arg = ast::value::MakeString("a");
  ast::value::Append(list_type, arg, &label_ast_);
  label = TFGraphExporter::NodeLabel(tag, label_ast_);
  EXPECT_EQ("foo/a", label);
  arg = ast::value::MakeString("b");
  ast::value::Append(list_type, arg, &label_ast_);
  label = TFGraphExporter::NodeLabel(tag, label_ast_);
  EXPECT_EQ("foo/a/b", label);
  arg = ast::value::MakeString("c");
  ast::value::Append(list_type, arg, &label_ast_);
  label = TFGraphExporter::NodeLabel(tag, label_ast_);
  EXPECT_EQ("foo/a/b/c", label);
}

TEST_F(TFGraphExporterTest, ExportsTFNode) {
  // Create the graph with edges:
  //   "a" -> "a/b"
  //   "a/b" -> "a/b/c"
  //   "a/b/c" -> "a".
  util::Status s = Initialize(&graph_);
  TFGraphExporter exporter(graph_);
  EXPECT_TRUE(s.ok());
  NodeId n1 = AddNode(kNodeLabelTag_, ast::value::MakeString("a"));
  NodeId n2 = AddNode(kNodeLabelTag_, ast::value::MakeString("a/b"));
  NodeId n3 = AddNode(kNodeLabelTag_, ast::value::MakeString("a/b/c"));
  // Get the TensorFlow graph.
  tensorflow::GraphDef tf_graph = exporter.TFGraph();
  // The graph should have three nodes.
  EXPECT_EQ(3, tf_graph.node_size());
  // There are no edges in the graph, so each node has no predecessors.
  for (tensorflow::NodeDef node : tf_graph.node()) {
    EXPECT_EQ(0, node.input_size());
  }
  AddEdge(n1, n2, ast::kPrecedesTag, ast::value::MakeNull());
  AddEdge(n2, n3, ast::kPrecedesTag, ast::value::MakeNull());
  AddEdge(n3, n1, ast::kPrecedesTag, ast::value::MakeNull());
  // The graph should have three nodes.
  // Every node should now have one predecessor.
  tf_graph = exporter.TFGraph();
  for (tensorflow::NodeDef node : tf_graph.node()) {
    EXPECT_EQ(1, node.input_size());
  }
  std::cout << exporter.TFGraphAsString() << std::endl;
}

}  // unnamed namespace
}  // namespace tervuren
