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

#include "labeled_graph.h"

#include <set>
#include <utility>

#include "third_party/logle/ast.pb.h"
#include "base/string.h"
#include "gtest.h"
#include "type.h"
#include "type_checker.h"
#include "util/status.h"
#include "value.h"
#include "value_checker.h"

namespace tervuren {
namespace {

using ast::type::Types;
namespace type = ast::type;
namespace value = ast::value;

static TaggedAST TempLabel() {
  AST val = type::MakeString("Value", true);
  TaggedAST label;
  label.set_tag("Name");
  label.set_allocated_ast(&val);
  return label;
}

// Test that calling a LabeledGraph method before a successful call to
// Initialize causes a crash.
// Uninitialized call to GetNodeTypes().
TEST(LabeledGraphDeathTest, UninitializedGetNodeTypes) {
  LabeledGraph graph;
  Types node_types;
  EXPECT_DEATH({ node_types = graph.GetNodeTypes(); }, ".*");
}

// Uninitialized call to GetUniqueNodeTags().
TEST(LabeledGraphDeathTest, UninitializedGetUniqueNodeTags) {
  LabeledGraph graph;
  set<string> tags;
  EXPECT_DEATH({ tags = graph.GetUniqueNodeTags(); }, ".*");
}

// Uninitialized call to GetNodeType.
TEST(LabeledGraphDeathTest, UninitializedGetNode) {
  LabeledGraph graph;
  std::pair<bool, AST> result;
  EXPECT_DEATH({ result = graph.GetNodeType(""); }, ".*");
}

// Uninitialized call to GetEdgeTypes().
TEST(LabeledGraphDeathTest, UninitializedGetEdgeTypes) {
  LabeledGraph graph;
  Types edge_types;
  EXPECT_DEATH({ edge_types = graph.GetEdgeTypes(); }, ".*");
}

// Uninitialized call to GetUniqueEdgeTags().
TEST(LabeledGraphDeathTest, UninitializedGetUniqueEdgeTags) {
  LabeledGraph graph;
  set<string> tags;
  EXPECT_DEATH({ tags = graph.GetUniqueEdgeTags(); }, ".*");
}

// Uninitialized call to GetGraphType().
TEST(LabeledGraphDeathTest, UninitializedGetGraphType) {
  LabeledGraph graph;
  AST graph_type;
  EXPECT_DEATH({ graph_type = graph.GetGraphType(); }, ".*");
}

// Uninitialized call to GetEdgeType.
TEST(LabeledGraphDeathTest, UninitializedGetEdge) {
  LabeledGraph graph;
  std::pair<bool, AST> result;
  EXPECT_DEATH({ result = graph.GetEdgeType(""); }, ".*");
}

// Uninitialized call to FindOrAddNode.
TEST(LabeledGraphDeathTest, UninitializedFindOrAddNode) {
  LabeledGraph graph;
  EXPECT_DEATH({ graph.FindOrAddNode(TempLabel()); }, ".*");
}

// Uninitialized call to FindOrAddEdge.
TEST(LabeledGraphDeathTest, UninitializedFindOrAddEdge) {
  LabeledGraph graph;
  EXPECT_DEATH({ graph.FindOrAddEdge(0, 0, TempLabel()); }, ".*");
}

// Uninitialized call to HasNode
TEST(LabeledGraphDeathTest, UninitializedHasNode) {
  LabeledGraph graph;
  EXPECT_DEATH({ graph.HasNode(0); }, ".*");
}

// Uninitialized call to GetNodeLabel.
TEST(LabeledGraphDeathTest, UninitializedGetNodeLabel) {
  LabeledGraph graph;
  EXPECT_DEATH({ graph.GetNodeLabel(0); }, ".*");
}

// Uninitialized call to GetGraphLabel.
TEST(LabeledGraphDeathTest, UninitializedGetGraphLabel) {
  LabeledGraph graph;
  EXPECT_DEATH({ graph.GetGraphLabel(); }, ".*");
}

// Uninitialized call to GetNodes.
TEST(LabeledGraphDeathTest, UninitializedGetNodes) {
  LabeledGraph graph;
  set<NodeId> nodes;
  EXPECT_DEATH({ nodes = graph.GetNodes(TempLabel()); }, ".*");
}

// Uninitialized call to GetEdges.
TEST(LabeledGraphDeathTest, UninitializedGetEdges) {
  LabeledGraph graph;
  set<EdgeId> edges;
  EXPECT_DEATH({ edges = graph.GetEdges(TempLabel()); }, ".*");
}

// Uninitialized call to NumNodeTypes.
TEST(LabeledGraphDeathTest, UninitializedNumNodetypes) {
  LabeledGraph graph;
  int num_node_types;
  EXPECT_DEATH({ num_node_types = graph.NumNodeTypes(); }, ".*");
}

// Uninitialized call to NumUniqueNodeTypes.
TEST(LabeledGraphDeathTest, UninitializedUniqueNodeTypes) {
  LabeledGraph graph;
  int unique_types;
  EXPECT_DEATH({ unique_types = graph.NumUniqueNodeTypes(); }, ".*");
}

// Uninitialized call to NumNodes.
TEST(LabeledGraphDeathTest, UninitializedNumNodes) {
  LabeledGraph graph;
  int num_nodes;
  EXPECT_DEATH({ num_nodes = graph.NumNodes(); }, ".*");
}

// Uninitialized call to NumLabeledNodes.
TEST(LabeledGraphDeathTest, UninitializedNumLabeledNodes) {
  LabeledGraph graph;
  int num_nodes;
  EXPECT_DEATH({ num_nodes = graph.NumLabeledNodes(TempLabel()); }, ".*");
}

// Uninitialized call to NumEdgeTypes.
TEST(LabeledGraphDeathTest, UninitializedNumEdgeTypes) {
  LabeledGraph graph;
  int num_types;
  EXPECT_DEATH({ num_types = graph.NumEdgeTypes(); }, ".*");
}

// Uninitialized call to NumEdges.
TEST(LabeledGraphDeathTest, UninitializedNumEdges) {
  LabeledGraph graph;
  int num_edges;
  EXPECT_DEATH({ num_edges = graph.NumEdges(); }, ".*");
}

// Uninitialized call to NumEdges.
TEST(LabeledGraphDeathTest, UninitializedNumLabeledEdges) {
  LabeledGraph graph;
  int num_edges;
  EXPECT_DEATH({ num_edges = graph.NumLabeledEdges(TempLabel()); }, ".*");
}

class LabeledGraphTest : public ::testing::Test {
 protected:
  LabeledGraph graph_;
};

// The tests that follow check cases when initialization should succeed/fail.
// Do not initialize if all arguments are empty.
TEST_F(LabeledGraphTest, RejectsAllEmptyTypes) {
  AST graph_type;
  EXPECT_FALSE(graph_.Initialize(Types(), set<string>(), Types(), set<string>(),
                                 graph_type).ok());
}

// Do not initialize if the graph type is malformed.
TEST_F(LabeledGraphTest, RejectMalformedGraphType) {
  AST graph_type = type::MakeString("name", false);
  graph_type.clear_is_nullable();
  EXPECT_FALSE(graph_.Initialize(Types(), set<string>(), Types(), set<string>(),
                                 graph_type).ok());
}

// Do not initialize if the set of node types contains a malformed type.
TEST_F(LabeledGraphTest, RejectsMalformedNodeType) {
  AST node_type;
  Types node_types;
  // Add an AST that is not a type to node_types.
  node_types.insert({"Event", node_type});
  AST graph_type = type::MakeString("name", false);
  EXPECT_FALSE(graph_.Initialize(node_types, set<string>(), Types(),
                                 set<string>(), graph_type).ok());
}

// Do not initialize if the set of edge types contains a malformed type.
TEST_F(LabeledGraphTest, RejectsMalformedEdgeType) {
  AST edge_type;
  Types edge_types;
  edge_types.insert({"Download", edge_type});
  AST graph_type = type::MakeString("name", false);
  EXPECT_FALSE(graph_.Initialize(Types(), set<string>(), edge_types,
                                 set<string>(), graph_type).ok());
}

// In cases where initialization succeeds, tested next, the graph obtained
// should be empty and should return appropriate information about types.
// If initialization succeeds, the graph is empty.
TEST_F(LabeledGraphTest, AcceptsOnlyNonEmptyGraphType) {
  AST graph_type = type::MakeString("name", false);
  EXPECT_TRUE(graph_.Initialize(Types(), set<string>(), Types(), set<string>(),
                                graph_type).ok());
  EXPECT_EQ(0, graph_.NumNodeTypes());
  EXPECT_EQ(0, graph_.NumUniqueNodeTypes());
  EXPECT_EQ(0, graph_.NumEdgeTypes());
  EXPECT_EQ(0, graph_.NumUniqueEdgeTypes());
  EXPECT_EQ(0, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
}

// Initialize with a non-empty node type.
TEST_F(LabeledGraphTest, AcceptsNonEmptyNodeAndGraphTypes) {
  AST node_type = type::MakeInt("EventID", false);
  Types node_types;
  node_types.insert({"Event", node_type});
  AST graph_type = type::MakeString("name", false);
  EXPECT_TRUE(graph_.Initialize(node_types, set<string>(), Types(),
                                set<string>(), graph_type).ok());
  EXPECT_EQ(1, graph_.NumNodeTypes());
  EXPECT_EQ(0, graph_.NumUniqueNodeTypes());
  EXPECT_EQ(0, graph_.NumEdgeTypes());
  EXPECT_EQ(0, graph_.NumUniqueEdgeTypes());
  EXPECT_EQ(0, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
}

// Initialize with a non-empty edge type.
TEST_F(LabeledGraphTest, AcceptsNonEmptyEdgeAndGraphTypes) {
  AST edge_type = type::MakeBool("isDownload", false);
  Types edge_types;
  edge_types.insert({"Download", edge_type});
  AST graph_type = type::MakeString("name", false);
  EXPECT_TRUE(graph_.Initialize(Types(), set<string>(), edge_types,
                                set<string>(), graph_type).ok());
  EXPECT_EQ(0, graph_.NumNodeTypes());
  EXPECT_EQ(0, graph_.NumUniqueNodeTypes());
  EXPECT_EQ(1, graph_.NumEdgeTypes());
  EXPECT_EQ(0, graph_.NumUniqueEdgeTypes());
  EXPECT_EQ(0, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
}

// Initialize with a non-empty node and edge type but no unique tags.
TEST_F(LabeledGraphTest, AcceptsAllNonEmptyTypesAndEmptyUniqueTag) {
  AST node_type = type::MakeInt("EventID", false);
  Types node_types;
  node_types.insert({"Event", node_type});
  AST edge_type = type::MakeBool("isDownload", false);
  Types edge_types;
  edge_types.insert({"Download", edge_type});
  AST graph_type = type::MakeString("name", false);
  EXPECT_TRUE(graph_.Initialize(node_types, set<string>(), edge_types,
                                set<string>(), graph_type).ok());
  EXPECT_EQ(1, graph_.NumNodeTypes());
  EXPECT_EQ(0, graph_.NumUniqueNodeTypes());
  EXPECT_EQ(1, graph_.NumEdgeTypes());
  EXPECT_EQ(0, graph_.NumUniqueEdgeTypes());
  EXPECT_EQ(0, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
}

// Initialize with non-empty node and  edge types, and unique node tags.
TEST_F(LabeledGraphTest, AcceptsAllNonEmptyTypesAndUniqueNodeTags) {
  AST node_type = type::MakeInt("EventID", false);
  Types node_types;
  node_types.insert({"Event", node_type});
  set<string> tags;
  tags.insert("Event");
  AST edge_type = type::MakeBool("isDownload", false);
  Types edge_types;
  edge_types.insert({"Download", edge_type});
  AST graph_type = type::MakeString("name", false);
  EXPECT_TRUE(graph_.Initialize(node_types, tags, edge_types, set<string>(),
                                graph_type).ok());
  EXPECT_EQ(1, graph_.NumNodeTypes());
  EXPECT_EQ(1, graph_.NumUniqueNodeTypes());
  EXPECT_EQ(1, graph_.NumEdgeTypes());
  EXPECT_EQ(0, graph_.NumUniqueEdgeTypes());
  EXPECT_EQ(0, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
}

// Initialize with non-empty node and  edge types, and unique edge tags.
TEST_F(LabeledGraphTest, AcceptsAllNonEmptyTypesAndUniqueEdgeTags) {
  AST node_type = type::MakeInt("EventID", false);
  Types node_types;
  node_types.insert({"Event", node_type});
  set<string> tags;
  tags.insert("Event");
  AST edge_type = type::MakeBool("isDownload", false);
  Types edge_types;
  edge_types.insert({"Download", edge_type});
  AST graph_type = type::MakeString("name", false);
  EXPECT_TRUE(graph_.Initialize(node_types, set<string>(), edge_types, tags,
                                graph_type).ok());
  EXPECT_EQ(1, graph_.NumNodeTypes());
  EXPECT_EQ(0, graph_.NumUniqueNodeTypes());
  EXPECT_EQ(1, graph_.NumEdgeTypes());
  EXPECT_EQ(1, graph_.NumUniqueEdgeTypes());
  EXPECT_EQ(0, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
}

// Initialize with non-empty node and  edge types, and unique tags.
TEST_F(LabeledGraphTest, AcceptsAllNonEmptyTypesAndUniqueTags) {
  AST node_type = type::MakeInt("EventID", false);
  Types node_types;
  node_types.insert({"Event", node_type});
  set<string> tags;
  tags.insert("Event");
  AST edge_type = type::MakeBool("isDownload", false);
  Types edge_types;
  edge_types.insert({"Download", edge_type});
  AST graph_type = type::MakeString("name", false);
  EXPECT_TRUE(
      graph_.Initialize(node_types, tags, edge_types, tags, graph_type).ok());
  EXPECT_EQ(1, graph_.NumNodeTypes());
  EXPECT_EQ(1, graph_.NumUniqueNodeTypes());
  EXPECT_EQ(1, graph_.NumEdgeTypes());
  EXPECT_EQ(1, graph_.NumUniqueEdgeTypes());
  EXPECT_EQ(0, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
}

// Initialize graph with two node types, one edge type and a graph type. The
// node type Event is an event id and the type File is a filename. The edge
// type Relation is a string indicating the relation between nodes and the
// graph type is a string for the system name.
util::Status Initialize(LabeledGraph* graph) {
  AST node_type = type::MakeInt("EventID", false);
  Types node_types;
  node_types.insert({"Event", node_type});
  node_type = type::MakeString("Filename", false);
  node_types.insert({"File", node_type});
  set<string> node_tags;
  node_tags.insert("File");
  AST edge_type = type::MakeString("Info", false);
  Types edge_types;
  edge_types.insert({"Relation", edge_type});
  edge_type = type::MakeInt("Number", false);
  edge_types.insert({"Frequency", edge_type});
  set<string> edge_tags;
  edge_tags.insert("Frequency");
  AST graph_type = type::MakeString("System", false);
  return graph->Initialize(node_types, node_tags, edge_types, edge_tags,
                           graph_type);
}

// Test retrieval of node types.
TEST_F(LabeledGraphTest, TestGetNodeTypes) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  Types node_types = graph_.GetNodeTypes();
  EXPECT_EQ(2, node_types.size());
  std::pair<bool, AST> result = graph_.GetNodeType("Event");
  EXPECT_TRUE(result.first);
  auto type_it = node_types.find("Event");
  EXPECT_TRUE(value::Isomorphic(type_it->second, result.second));
  result = graph_.GetNodeType("Info");
  EXPECT_FALSE(result.first);
  set<string> tags = graph_.GetUniqueNodeTags();
  EXPECT_EQ(1, tags.size());
  EXPECT_EQ("File", *tags.begin());
}

// Test retrieval of edge types.
TEST_F(LabeledGraphTest, TestGetEdgeTypes) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  Types edge_types = graph_.GetEdgeTypes();
  EXPECT_EQ(2, edge_types.size());
  std::pair<bool, AST> result = graph_.GetEdgeType("Relation");
  EXPECT_TRUE(result.first);
  auto type_it = edge_types.find("Relation");
  EXPECT_TRUE(value::Isomorphic(type_it->second, result.second));
  result = graph_.GetEdgeType("Info");
  EXPECT_FALSE(result.first);
  set<string> tags = graph_.GetUniqueEdgeTags();
  EXPECT_EQ(1, tags.size());
  EXPECT_EQ("Frequency", *tags.begin());
}

// Test that the graph label is set correctly.
TEST_F(LabeledGraphTest, GraphLabelSetCorrectly) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  AST type = ast::type::MakeString("System", false);
  AST label = ast::value::MakeString("Goobuntu");
  string err;
  EXPECT_TRUE(type::IsType(graph_.GetGraphType(), &err));
  EXPECT_TRUE(value::Isomorphic(type, graph_.GetGraphType()));
  graph_.SetGraphLabel(label);
  EXPECT_TRUE(value::Isomorphic(label, graph_.GetGraphLabel()));
}

TEST(LabeledGraphDeathTest, GraphLabelTypeMismatch) {
  LabeledGraph graph;
  EXPECT_TRUE(Initialize(&graph).ok());
  AST label = type::MakeInt("Num", false);
  label.clear_is_nullable();
  label.clear_name();
  label.mutable_p_ast()->mutable_val()->set_int_val(5);
  EXPECT_DEATH({ graph.SetGraphLabel(label); }, ".*");
}

// Helper method that constructs an event label.
TaggedAST GetIntLabel(const string& tag, int val) {
  AST ast = type::MakeInt("foo", false);
  ast.clear_is_nullable();
  ast.clear_name();
  ast.mutable_p_ast()->mutable_val()->set_int_val(val);
  TaggedAST label;
  label.set_tag(tag);
  *label.mutable_ast() = ast;
  return label;
}

// Helper method that constructs a string label.
TaggedAST GetStringLabel(const string& tag, const string& name) {
  AST ast = type::MakeString("", false);
  ast.clear_is_nullable();
  ast.clear_name();
  ast.mutable_p_ast()->mutable_val()->set_string_val(name);
  TaggedAST label;
  label.set_tag(tag);
  *label.mutable_ast() = ast;
  return label;
}

// Test the unique label method.
TEST_F(LabeledGraphTest, TestUniqueLabels) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  EXPECT_FALSE(graph_.IsUniqueNodeType(GetIntLabel("Event", 5)));
  EXPECT_TRUE(graph_.IsUniqueNodeType(GetStringLabel("File", "foo.txt")));
  EXPECT_TRUE(graph_.IsUniqueEdgeType(GetStringLabel("Frequency", "Download")));
  EXPECT_FALSE(graph_.IsUniqueEdgeType(GetStringLabel("NonRelation", "foo")));
}

// The begin and end iterators for the node and edge sets of the empty graph
// should coincide.
TEST_F(LabeledGraphTest, IteratorsOfEmptyGraph) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  EXPECT_EQ(graph_.NodeSetBegin(), graph_.NodeSetEnd());
  EXPECT_EQ(graph_.EdgeSetBegin(), graph_.EdgeSetEnd());
}

// Create a graph with a single node.
TEST_F(LabeledGraphTest, CreateSingleNodeGraph) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  TaggedAST label = GetIntLabel("Event", 5);
  NodeId node_id = graph_.FindOrAddNode(label);
  // Graph should have one node with the given label.
  EXPECT_EQ(1, graph_.NumNodes());
  EXPECT_EQ(1, graph_.NumLabeledNodes(label));
  set<NodeId> nodes = graph_.GetNodes(label);
  EXPECT_EQ(1, nodes.size());
  EXPECT_EQ(node_id, *nodes.begin());
  TaggedAST node_label = graph_.GetNodeLabel(node_id);
  EXPECT_EQ(label.tag(), node_label.tag());
  EXPECT_TRUE(value::Isomorphic(label.ast(), node_label.ast()));
  // Begin and end iterators of the node set should be one element apart.
  NodeIterator node_it = graph_.NodeSetBegin();
  EXPECT_NE(graph_.NodeSetEnd(), node_it);
  EXPECT_EQ(graph_.NodeSetEnd(), ++node_it);
  // The graph should have no edges and the begin and end iterators of the edge
  // set should coincide.
  EXPECT_EQ(0, graph_.NumEdges());
  EXPECT_EQ(graph_.EdgeSetBegin(), graph_.EdgeSetEnd());
  EXPECT_EQ(0, graph_.NumLabeledEdges(label));
}

// Check that queries do not return the same results for all inputs.
TEST_F(LabeledGraphTest, QuerySingleNodeGraph) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  NodeId node_id = graph_.FindOrAddNode(GetIntLabel("Event", 5));
  // Graph should not have nodes with other labels.
  TaggedAST non_label = GetIntLabel("Event", 4);
  // EXPECT_TRUE(graph_.GetNodes(non_label).empty());
  EXPECT_EQ(0, graph_.GetNodes(non_label).size());
  EXPECT_FALSE(
      value::Isomorphic(non_label.ast(), graph_.GetNodeLabel(node_id).ast()));
}

// Calling FindOrAddNode multiple times with a unique label type should not
// increase the number of nodes in the graph.
TEST_F(LabeledGraphTest, CreateSingleUniqueNodeGraph) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  TaggedAST label = GetStringLabel("File", "foo.txt");
  NodeId node1 = graph_.FindOrAddNode(label);
  NodeId node2 = graph_.FindOrAddNode(label);
  // Graph should have one node with the given label.
  EXPECT_EQ(node1, node2);
  EXPECT_EQ(1, graph_.NumNodes());
  EXPECT_EQ(1, graph_.NumLabeledNodes(label));
  set<NodeId> nodes = graph_.GetNodes(label);
  EXPECT_EQ(1, nodes.size());
  EXPECT_EQ(*nodes.begin(), node1);
  TaggedAST node_label = graph_.GetNodeLabel(node1);
  EXPECT_EQ(label.tag(), node_label.tag());
  EXPECT_TRUE(value::Isomorphic(label.ast(), node_label.ast()));
  // The graph should have no edges.
  EXPECT_EQ(0, graph_.NumEdges());
  EXPECT_EQ(0, graph_.NumLabeledEdges(label));
}

// Test properties with a graph with two nodes and no edges.
TEST_F(LabeledGraphTest, TwoNodeNoEdgeGraph) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  TaggedAST event_label = GetIntLabel("Event", 5);
  NodeId event_id = graph_.FindOrAddNode(event_label);
  TaggedAST file_label = GetStringLabel("File", "foo.txt");
  NodeId file_id = graph_.FindOrAddNode(file_label);
  // Graph should have two nodes with the given labels.
  EXPECT_EQ(2, graph_.NumNodes());
  EXPECT_EQ(1, graph_.NumLabeledNodes(event_label));
  set<NodeId> nodes = graph_.GetNodes(event_label);
  EXPECT_EQ(1, nodes.size());
  EXPECT_EQ(1, graph_.NumLabeledNodes(file_label));
  EXPECT_EQ(*nodes.begin(), event_id);
  nodes = graph_.GetNodes(file_label);
  EXPECT_EQ(1, nodes.size());
  EXPECT_EQ(*nodes.begin(), file_id);
  // Begin and end iterators of the node set should be two elements apart.
  NodeIterator node_it = graph_.NodeSetBegin();
  EXPECT_NE(graph_.NodeSetEnd(), node_it);
  EXPECT_NE(graph_.NodeSetEnd(), ++node_it);
  EXPECT_EQ(graph_.NodeSetEnd(), ++node_it);
  // The graph should have no edges and the begin and end iterators should
  // coincide.
  EXPECT_EQ(0, graph_.NumEdges());
  EXPECT_EQ(0, graph_.NumLabeledEdges(event_label));
  EXPECT_EQ(0, graph_.NumLabeledEdges(file_label));
  EXPECT_EQ(graph_.EdgeSetBegin(), graph_.EdgeSetEnd());
}

// Test methods that manipulate edges.
// Check single node with a self loop.
TEST_F(LabeledGraphTest, SingleNodeSelfLoop) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  TaggedAST node_label = GetIntLabel("Event", 5);
  TaggedAST edge_label = GetStringLabel("Relation", "Self-loop");
  NodeId node_id = graph_.FindOrAddNode(node_label);
  EdgeId edge_id = graph_.FindOrAddEdge(node_id, node_id, edge_label);
  // Check node properties of the graph.
  EXPECT_EQ(1, graph_.NumNodes());
  EXPECT_EQ(1, graph_.NumLabeledNodes(node_label));
  EXPECT_EQ(0, graph_.NumLabeledNodes(edge_label));
  EXPECT_EQ(*(graph_.GetNodes(node_label).begin()), node_id);
  // Check edge properties of the graph.
  EXPECT_EQ(1, graph_.NumEdges());
  EXPECT_EQ(0, graph_.NumLabeledEdges(node_label));
  EXPECT_EQ(1, graph_.NumLabeledEdges(edge_label));
  EXPECT_EQ(0, graph_.GetEdges(node_label).size());
  EXPECT_EQ(1, graph_.GetEdges(edge_label).size());
  EXPECT_EQ(*(graph_.GetEdges(edge_label).begin()), edge_id);
  // Check that the edge has the right source and target.
  EXPECT_EQ(graph_.Source(edge_id), node_id);
  EXPECT_EQ(graph_.Target(edge_id), node_id);
  // Begin and end iterators of the edge set should be one element apart.
  EdgeIterator edge_it = graph_.EdgeSetBegin();
  EXPECT_NE(graph_.EdgeSetEnd(), edge_it);
  EXPECT_EQ(graph_.EdgeSetEnd(), ++edge_it);
}

// Check a single node with multiple unique and non-unique edges.
TEST_F(LabeledGraphTest, SingleNodeMultipleLoops) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  TaggedAST node_label = GetIntLabel("Event", 5);
  TaggedAST edge1_label = GetStringLabel("Relation", "Self-loop");
  TaggedAST edge2_label = GetIntLabel("Frequency", 2);
  NodeId node_id = graph_.FindOrAddNode(node_label);
  // Add the non-unique edge twice.
  EdgeId edge1_id = graph_.FindOrAddEdge(node_id, node_id, edge1_label);
  EdgeId edge2_id = graph_.FindOrAddEdge(node_id, node_id, edge1_label);
  EXPECT_NE(edge1_id, edge2_id);
  // Add the unique edge twice.
  EdgeId edge3_id = graph_.FindOrAddEdge(node_id, node_id, edge2_label);
  EdgeId edge4_id = graph_.FindOrAddEdge(node_id, node_id, edge2_label);
  EXPECT_NE(edge1_id, edge2_id);
  EXPECT_NE(edge2_id, edge3_id);
  EXPECT_EQ(edge3_id, edge4_id);
  // The graph should have one node and three edges.
  EXPECT_EQ(1, graph_.NumNodes());
  EXPECT_EQ(1, graph_.NumLabeledNodes(node_label));
  EXPECT_EQ(0, graph_.NumLabeledNodes(edge1_label));
  EXPECT_EQ(*(graph_.GetNodes(node_label).begin()), node_id);
  // Check edge properties of the graph.
  EXPECT_EQ(3, graph_.NumEdges());
  EXPECT_EQ(0, graph_.NumLabeledEdges(node_label));
  EXPECT_EQ(2, graph_.NumLabeledEdges(edge1_label));
  EXPECT_EQ(1, graph_.NumLabeledEdges(edge2_label));
  EXPECT_EQ(2, graph_.GetEdges(edge1_label).size());
  EXPECT_EQ(1, graph_.GetEdges(edge2_label).size());
  EXPECT_EQ(*(graph_.GetEdges(edge2_label).begin()), edge3_id);
}

// Test properties with a graph with two nodes and one edge.
TEST_F(LabeledGraphTest, TwoNodesOneEdgeGraph) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  TaggedAST event_label = GetIntLabel("Event", 5);
  NodeId event_id = graph_.FindOrAddNode(event_label);
  TaggedAST file_label = GetStringLabel("File", "foo.txt");
  NodeId file_id = graph_.FindOrAddNode(file_label);
  TaggedAST edge_label = GetStringLabel("Relation", "Modified");
  EdgeId edge_id = graph_.FindOrAddEdge(event_id, file_id, edge_label);
  // Check edge properties of the graph.
  EXPECT_EQ(1, graph_.NumEdges());
  EXPECT_EQ(1, graph_.NumLabeledEdges(edge_label));
  EXPECT_EQ(1, graph_.GetEdges(edge_label).size());
  EXPECT_EQ(*(graph_.GetEdges(edge_label).begin()), edge_id);
  // Check the source and target of the edge.
  EXPECT_EQ(graph_.Source(edge_id), event_id);
  EXPECT_EQ(graph_.Target(edge_id), file_id);
}

// Test properties with a graph with two nodes and two edges in opposite
// directions.
TEST_F(LabeledGraphTest, TwoNodesTwoEdgesGraph) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  TaggedAST event_label = GetIntLabel("Event", 5);
  NodeId event_id = graph_.FindOrAddNode(event_label);
  TaggedAST file_label = GetStringLabel("File", "foo.txt");
  NodeId file_id = graph_.FindOrAddNode(file_label);
  TaggedAST modifies_label = GetStringLabel("Relation", "Modifies");
  TaggedAST modifier_label = GetStringLabel("Relation", "Modifier");
  EdgeId modifies_edge =
      graph_.FindOrAddEdge(event_id, file_id, modifies_label);
  EdgeId modifier_edge =
      graph_.FindOrAddEdge(file_id, event_id, modifier_label);
  // Check edge properties of the graph.
  EXPECT_EQ(2, graph_.NumEdges());
  EXPECT_EQ(1, graph_.NumLabeledEdges(modifies_label));
  EXPECT_EQ(1, graph_.NumLabeledEdges(modifier_label));
  EXPECT_EQ(*(graph_.GetEdges(modifies_label).begin()), modifies_edge);
  EXPECT_EQ(*(graph_.GetEdges(modifier_label).begin()), modifier_edge);
  // Begin and end iterators of the edge set should be two elements apart.
  EdgeIterator edge_it = graph_.EdgeSetBegin();
  EXPECT_NE(graph_.EdgeSetEnd(), edge_it);
  EXPECT_NE(graph_.EdgeSetEnd(), ++edge_it);
  EXPECT_EQ(graph_.EdgeSetEnd(), ++edge_it);
}

// Test properties with a graph with two nodes and two edges in the same
// direction.
TEST_F(LabeledGraphTest, TwoNodesTwoEdgesSameOrientation) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  TaggedAST event_label = GetIntLabel("Event", 5);
  NodeId event_id = graph_.FindOrAddNode(event_label);
  TaggedAST file_label = GetStringLabel("File", "foo.txt");
  NodeId file_id = graph_.FindOrAddNode(file_label);
  TaggedAST relation_label = GetStringLabel("Relation", "Modifies");
  TaggedAST frequency_label = GetIntLabel("Frequency", 2);
  EdgeId relation_edge =
      graph_.FindOrAddEdge(event_id, file_id, relation_label);
  EdgeId frequency_edge =
      graph_.FindOrAddEdge(event_id, file_id, frequency_label);
  EdgeId frequency2_edge =
      graph_.FindOrAddEdge(event_id, file_id, frequency_label);
  EXPECT_NE(relation_edge, frequency_edge);
  EXPECT_EQ(frequency_edge, frequency2_edge);
  // Check edge properties of the graph.
  EXPECT_EQ(2, graph_.NumEdges());
  EXPECT_EQ(1, graph_.NumLabeledEdges(relation_label));
  EXPECT_EQ(1, graph_.NumLabeledEdges(frequency_label));
  EXPECT_EQ(*(graph_.GetEdges(relation_label).begin()), relation_edge);
  EXPECT_EQ(*(graph_.GetEdges(frequency_label).begin()), frequency_edge);
}

// Test functions for accessing predecessors and successors.
TEST_F(LabeledGraphTest, GetNeighbors) {
  // Create this graph:  0 -> 1, 0 -> 2, 1 -> 2
  ASSERT_TRUE(Initialize(&graph_).ok());
  vector<TaggedAST> label = {GetIntLabel("Event", 0), GetIntLabel("Event", 1),
                             GetIntLabel("Event", 2)};
  vector<NodeId> node_id;
  node_id.resize(3);
  node_id[0] = graph_.FindOrAddNode(label[0]);
  node_id[1] = graph_.FindOrAddNode(label[1]);
  node_id[2] = graph_.FindOrAddNode(label[2]);
  TaggedAST edge_label = GetStringLabel("Relation", "Less-Than");
  graph_.FindOrAddEdge(node_id[0], node_id[1], edge_label);
  graph_.FindOrAddEdge(node_id[0], node_id[2], edge_label);
  graph_.FindOrAddEdge(node_id[1], node_id[2], edge_label);

  // Create a new, empty graph and check that predecessor and successor queries
  // return an empty set.
  LabeledGraph empty_graph;
  ASSERT_TRUE(Initialize(&empty_graph).ok());
  EXPECT_FALSE(empty_graph.HasNode(node_id[0]));
  // Check the predecessors of nodes in 'graph_'.
  set<NodeId> node_set;
  InEdgeIterator in_edge_it;
  // Check that predecessors(0) == {}, the empty set.
  EXPECT_TRUE(graph_.HasNode(node_id[0]));
  EXPECT_EQ(graph_.InEdgeBegin(node_id[0]), graph_.InEdgeEnd(node_id[0]));
  EXPECT_EQ(graph_.GetPredecessors(node_id[0]), node_set);
  EXPECT_EQ(graph_.GetLabelPredecessors(label[0]), node_set);
  // Check that predecessors(1) == {0}.
  node_set.insert(node_id[0]);
  in_edge_it = graph_.InEdgeBegin(node_id[1]);
  EXPECT_NE(in_edge_it, graph_.InEdgeEnd(node_id[1]));
  ++in_edge_it;
  EXPECT_EQ(in_edge_it, graph_.InEdgeEnd(node_id[1]));
  EXPECT_EQ(graph_.GetPredecessors(node_id[1]), node_set);
  EXPECT_EQ(graph_.GetLabelPredecessors(label[1]), node_set);
  // Check that predecessors(2) == {1, 2}
  node_set.insert(node_id[1]);
  in_edge_it = graph_.InEdgeBegin(node_id[2]);
  EXPECT_NE(in_edge_it, graph_.InEdgeEnd(node_id[2]));
  ++in_edge_it;
  EXPECT_NE(in_edge_it, graph_.InEdgeEnd(node_id[2]));
  ++in_edge_it;
  EXPECT_EQ(in_edge_it, graph_.InEdgeEnd(node_id[2]));
  EXPECT_EQ(graph_.GetPredecessors(node_id[2]), node_set);
  EXPECT_EQ(graph_.GetLabelPredecessors(label[2]), node_set);
  // Check successors of nodes in the graph above.
  node_set.clear();
  // Check that successors(2) == {}.
  EXPECT_EQ(graph_.OutEdgeBegin(node_id[2]), graph_.OutEdgeEnd(node_id[2]));
  EXPECT_EQ(graph_.GetSuccessors(node_id[2]), node_set);
  EXPECT_EQ(graph_.GetLabelSuccessors(label[2]), node_set);
  EXPECT_EQ(graph_.GetSuccessors(node_id[2]), node_set);
  // Check that successors(1) == {2}.
  node_set.insert(node_id[2]);
  OutEdgeIterator out_edge_it = graph_.OutEdgeBegin(node_id[1]);
  EXPECT_NE(out_edge_it, graph_.OutEdgeEnd(node_id[1]));
  ++out_edge_it;
  EXPECT_EQ(out_edge_it, graph_.OutEdgeEnd(node_id[1]));
  EXPECT_EQ(graph_.GetSuccessors(node_id[1]), node_set);
  EXPECT_EQ(graph_.GetLabelSuccessors(label[1]), node_set);
  // Check that successors(0) = {1,2}.
  node_set.insert(node_id[1]);
  out_edge_it = graph_.OutEdgeBegin(node_id[0]);
  EXPECT_NE(out_edge_it, graph_.OutEdgeEnd(node_id[0]));
  ++out_edge_it;
  EXPECT_NE(out_edge_it, graph_.OutEdgeEnd(node_id[0]));
  ++out_edge_it;
  EXPECT_EQ(out_edge_it, graph_.OutEdgeEnd(node_id[0]));
  EXPECT_EQ(graph_.GetSuccessors(node_id[0]), node_set);
  EXPECT_EQ(graph_.GetLabelSuccessors(label[0]), node_set);
}

TEST_F(LabeledGraphTest, UpdateNodeLabels) {
  ASSERT_TRUE(Initialize(&graph_).ok());
  TaggedAST event1_label = GetIntLabel("Event", 5);
  TaggedAST event2_label = GetIntLabel("Event", 12);
  NodeId event1_id = graph_.FindOrAddNode(event1_label);
  NodeId event2_id = graph_.FindOrAddNode(event2_label);
  TaggedAST foo_label = GetStringLabel("File", "foo.txt");
  graph_.FindOrAddNode(foo_label);
  // The graph should have three nodes and no edges.
  ASSERT_EQ(3, graph_.NumNodes());
  ASSERT_EQ(0, graph_.NumEdges());
  ASSERT_EQ(1, graph_.GetNodes(event1_label).size());
  ASSERT_EQ(1, graph_.GetNodes(event2_label).size());
  // After updating the label of event1 to be event2, there will be two event
  // nodes with the same label.
  graph_.UpdateNodeLabel(event1_id, event2_label);
  EXPECT_EQ(3, graph_.NumNodes());
  EXPECT_EQ(0, graph_.NumEdges());
  EXPECT_EQ(0, graph_.GetNodes(event1_label).size());
  EXPECT_EQ(2, graph_.GetNodes(event2_label).size());
  std::set<NodeId> event_nodes = graph_.GetNodes(event2_label);
  EXPECT_TRUE(event_nodes.find(event1_id) != event_nodes.end());
  EXPECT_TRUE(event_nodes.find(event2_id) != event_nodes.end());
  // Change the label of event1 to be a file different from foo.
  TaggedAST bar_label = GetStringLabel("File", "bar.txt");
  graph_.UpdateNodeLabel(event1_id, bar_label);
  EXPECT_EQ(3, graph_.NumNodes());
  EXPECT_EQ(0, graph_.GetNodes(event1_label).size());
  EXPECT_EQ(1, graph_.GetNodes(event2_label).size());
  EXPECT_EQ(1, graph_.GetNodes(foo_label).size());
  std::set<NodeId> bar_nodes = graph_.GetNodes(bar_label);
  EXPECT_EQ(1, bar_nodes.size());
  EXPECT_EQ(event1_id, *bar_nodes.begin());
}

// The label of a node cannot be changed to a unique label that already exists
// in the graph.
TEST_F(LabeledGraphTest, UniqueNodeUpdateClash) {
  ASSERT_TRUE(Initialize(&graph_).ok());
  TaggedAST foo_label = GetStringLabel("File", "foo.txt");
  TaggedAST bar_label = GetStringLabel("File", "bar.txt");
  graph_.FindOrAddNode(foo_label);
  NodeId bar_id = graph_.FindOrAddNode(bar_label);
  EXPECT_FALSE(graph_.UpdateNodeLabel(bar_id, foo_label).ok());
}

TEST_F(LabeledGraphTest, UpdateEdgeLabels) {
  ASSERT_TRUE(Initialize(&graph_).ok());
  // Construct the graph:
  //  [foo.txt]<--3--[Event:5]--[forks]-->[Event:12]
  TaggedAST event1_label = GetIntLabel("Event", 5);
  TaggedAST event2_label = GetIntLabel("Event", 12);
  NodeId event1_id = graph_.FindOrAddNode(event1_label);
  NodeId event2_id = graph_.FindOrAddNode(event2_label);
  TaggedAST fork_label = GetStringLabel("Relation", "forks");
  TaggedAST freq_five = GetIntLabel("Frequency", 5);
  EdgeId fork_edge1 = graph_.FindOrAddEdge(event1_id, event2_id, fork_label);
  EdgeId fork_edge2 = graph_.FindOrAddEdge(event1_id, event2_id, fork_label);
  TaggedAST foo_label = GetStringLabel("File", "foo.txt");
  NodeId file_id = graph_.FindOrAddNode(foo_label);
  EdgeId file_edge = graph_.FindOrAddEdge(event1_id, file_id, freq_five);
  graph_.FindOrAddEdge(event1_id, file_id, freq_five);
  // The graph should have three nodes and two edges.
  ASSERT_EQ(3, graph_.NumNodes());
  ASSERT_EQ(3, graph_.NumEdges());
  ASSERT_EQ(2, graph_.GetEdges(fork_label).size());
  // After updating the label of event1 to be event2, there will be two event
  // nodes with the same label.
  TaggedAST child_label = GetStringLabel("Relation", "child");
  graph_.UpdateEdgeLabel(fork_edge1, child_label);
  EXPECT_EQ(3, graph_.NumNodes());
  EXPECT_EQ(3, graph_.NumEdges());
  std::set<EdgeId> edges = graph_.GetEdges(fork_label);
  EXPECT_EQ(1, edges.size());
  EXPECT_EQ(fork_edge2, *edges.begin());
  edges = graph_.GetEdges(child_label);
  EXPECT_EQ(1, edges.size());
  EXPECT_EQ(fork_edge1, *edges.begin());
  EXPECT_EQ(1, graph_.GetEdges(freq_five).size());
  TaggedAST freq_two = GetIntLabel("Frequency", 2);
  graph_.UpdateEdgeLabel(file_edge, freq_two);
  EXPECT_EQ(0, graph_.GetEdges(freq_five).size());
  EXPECT_EQ(1, graph_.GetEdges(freq_two).size());
}

TEST_F(LabeledGraphTest, UniqueEdgeUpdateClash) {
  ASSERT_TRUE(Initialize(&graph_).ok());
  TaggedAST event_label = GetIntLabel("Event", 13);
  TaggedAST bar_label = GetStringLabel("File", "bar.txt");
  NodeId event_id = graph_.FindOrAddNode(event_label);
  NodeId file_id = graph_.FindOrAddNode(bar_label);
  TaggedAST freq1_label = GetIntLabel("Frequency", 31);
  TaggedAST freq2_label = GetIntLabel("Frequency", 175);
  graph_.FindOrAddEdge(event_id, file_id, freq1_label);
  EdgeId edge2_id = graph_.FindOrAddEdge(event_id, file_id, freq2_label);
  EXPECT_FALSE(graph_.UpdateEdgeLabel(edge2_id, freq1_label).ok());
}

}  // namespace
}  // namespace tervuren
