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

#include "graph_transformer.h"

#include <memory>

#include "gtest.h"
#include "test_graphs.h"
#include "value_checker.h"

namespace logle {
namespace {

TEST(GraphTransformerTest, DeleteNodeFromSingleEdge) {
  // Create the graph { 0 -> 1 } and obtain the identifiers for the two nodes in
  // the graph.
  test::WeightedGraph one_node;
  test::GetPathGraph(2, &one_node);
  ASSERT_EQ(2, one_node.NumNodes());
  ASSERT_EQ(1, one_node.NumEdges());
  const LabeledGraph& input_graph = *one_node.GetGraph();
  NodeIterator node_it = input_graph.NodeSetBegin();
  NodeId first_node = *node_it;
  ++node_it;
  NodeId second_node = *node_it;
  // If no nodes are deleted, the graph should not change.
  std::unique_ptr<LabeledGraph> graph1 = graph::DeleteNodes(input_graph, {});
  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(2, graph1->NumNodes());
  EXPECT_EQ(1, graph1->NumEdges());
  // If the first node is deleted, the graph will have one node and no edges.
  std::unique_ptr<LabeledGraph> graph2 =
      graph::DeleteNodes(input_graph, {first_node});
  EXPECT_TRUE(graph2 != nullptr);
  EXPECT_EQ(1, graph2->NumNodes());
  EXPECT_EQ(0, graph2->NumEdges());
  // If the second node is deleted, the graph will have one node and no edges.
  std::unique_ptr<LabeledGraph> graph3 =
      graph::DeleteNodes(input_graph, {second_node});
  EXPECT_TRUE(graph3 != nullptr);
  EXPECT_EQ(1, graph3->NumNodes());
  EXPECT_EQ(0, graph3->NumEdges());
  // The nodes in the graphs obtained by deleting the first node and deleting
  // the second node should have different labels.
  TaggedAST label2 = graph2->GetNodeLabel(*graph2->NodeSetBegin());
  TaggedAST label3 = graph3->GetNodeLabel(*graph3->NodeSetBegin());
  EXPECT_EQ(label2.tag(), label3.tag());
  EXPECT_TRUE(label2.has_ast());
  EXPECT_TRUE(label3.has_ast());
  EXPECT_FALSE(ast::value::Isomorphic(label2.ast(), label3.ast()));
  // Deleting both nodes results in the empty graph.
  std::unique_ptr<LabeledGraph> graph4 =
      graph::DeleteNodes(input_graph, {first_node, second_node});
  EXPECT_TRUE(graph4 != nullptr);
  EXPECT_EQ(0, graph4->NumNodes());
  EXPECT_EQ(0, graph4->NumEdges());
}

TEST(GraphTransformerTest, DeleteNodeFromCycle) {
  // Create the graph { 0 -> 1, 1 -> 2, 2 -> 0 }.
  test::WeightedGraph triangle;
  test::GetCycleGraph(3, &triangle);
  ASSERT_EQ(3, triangle.NumNodes());
  ASSERT_EQ(3, triangle.NumEdges());
  const LabeledGraph& input_graph = *triangle.GetGraph();
  NodeId zero = *triangle.GetNodes(0).begin();
  NodeId one = *triangle.GetNodes(1).begin();
  NodeId two = *triangle.GetNodes(2).begin();
  TaggedAST zero_label = input_graph.GetNodeLabel(zero);
  TaggedAST one_label = input_graph.GetNodeLabel(one);
  // Check that deleting '2' results in the graph { 0 -> 1 }.
  std::unique_ptr<LabeledGraph> graph1 = graph::DeleteNodes(input_graph, {two});
  // Check that the graph has two nodes and one edge.
  EXPECT_EQ(2, graph1->NumNodes());
  EXPECT_EQ(1, graph1->NumEdges());
  EdgeId edge_id = *graph1->EdgeSetBegin();
  // Check that the edge labels are { 0 -> 1 }.
  TaggedAST src_label = graph1->GetNodeLabel(graph1->Source(edge_id));
  TaggedAST tgt_label = graph1->GetNodeLabel(graph1->Target(edge_id));
  EXPECT_EQ(src_label.tag(), tgt_label.tag());
  EXPECT_TRUE(src_label.has_ast());
  EXPECT_TRUE(tgt_label.has_ast());
  EXPECT_TRUE(ast::value::Isomorphic(zero_label.ast(), src_label.ast()));
  EXPECT_TRUE(ast::value::Isomorphic(one_label.ast(), tgt_label.ast()));
  // Check that deleting {1, 2} results in a graph with one node and no edges.
  std::unique_ptr<LabeledGraph> graph2 =
      graph::DeleteNodes(input_graph, {one, two});
  EXPECT_EQ(1, graph2->NumNodes());
  EXPECT_EQ(0, graph2->NumEdges());
  src_label = graph2->GetNodeLabel(*graph2->NodeSetBegin());
  EXPECT_TRUE(ast::value::Isomorphic(zero_label.ast(), src_label.ast()));
}

}  // namespace
}  // namespace logle
