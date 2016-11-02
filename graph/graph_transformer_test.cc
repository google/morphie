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

#include "ast.h"
#include "gtest.h"
#include "morphism.h"
#include "test_graphs.h"
#include "type.h"
#include "util/string_utils.h"
#include "value.h"
#include "value_checker.h"

namespace morphie {
namespace graph {
namespace {
namespace type = ast::type;

const char* const kBlockTag = "Block";
const char* const kEdgeTag = "Edge";

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
  std::unique_ptr<Morphism> morphism1 = DeleteNodes(input_graph, {});
  EXPECT_TRUE(morphism1->HasOutputGraph());
  const LabeledGraph& graph1 = morphism1->Output();
  EXPECT_EQ(2, graph1.NumNodes());
  EXPECT_EQ(1, graph1.NumEdges());
  // If the first node is deleted, the graph will have one node and no edges.
  std::unique_ptr<Morphism> morphism2 = DeleteNodes(input_graph, {first_node});
  EXPECT_TRUE(morphism2->HasOutputGraph());
  const LabeledGraph& graph2 = morphism2->Output();
  EXPECT_EQ(1, graph2.NumNodes());
  EXPECT_EQ(0, graph2.NumEdges());
  // If the second node is deleted, the graph will have one node and no edges.
  std::unique_ptr<Morphism> morphism3 =
      DeleteNodes(input_graph, {second_node});
  EXPECT_TRUE(morphism3->HasOutputGraph());
  const LabeledGraph& graph3 = morphism3->Output();
  EXPECT_EQ(1, graph3.NumNodes());
  EXPECT_EQ(0, graph3.NumEdges());
  // The nodes in the graphs obtained by deleting the first node and deleting
  // the second node should have different labels.
  TaggedAST label2 = graph2.GetNodeLabel(*graph2.NodeSetBegin());
  TaggedAST label3 = graph3.GetNodeLabel(*graph3.NodeSetBegin());
  EXPECT_EQ(label2.tag(), label3.tag());
  EXPECT_TRUE(label2.has_ast());
  EXPECT_TRUE(label3.has_ast());
  EXPECT_FALSE(ast::value::Isomorphic(label2.ast(), label3.ast()));
  // Deleting both nodes results in the empty graph.
  std::unique_ptr<Morphism> morphism4 =
      DeleteNodes(input_graph, {first_node, second_node});
  EXPECT_TRUE(morphism4->HasOutputGraph());
  const LabeledGraph& graph4 = morphism4->Output();
  EXPECT_EQ(0, graph4.NumNodes());
  EXPECT_EQ(0, graph4.NumEdges());
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
  std::unique_ptr<Morphism> morphism1 = DeleteNodes(input_graph, {two});
  const LabeledGraph* graph1 = morphism1->MutableOutput();
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
  std::unique_ptr<Morphism> morphism2 = DeleteNodes(input_graph, {one, two});
  const LabeledGraph* graph2 = morphism2->MutableOutput();
  EXPECT_EQ(1, graph2->NumNodes());
  EXPECT_EQ(0, graph2->NumEdges());
  src_label = graph2->GetNodeLabel(*graph2->NodeSetBegin());
  EXPECT_TRUE(ast::value::Isomorphic(zero_label.ast(), src_label.ast()));
}

TEST(GraphTransformerTest, DeleteSingleEdge) {
  // Create the graph { 0 -> 1 } and test deletion of edges from it.
  test::WeightedGraph one_edge;
  test::GetPathGraph(2, &one_edge);
  ASSERT_EQ(2, one_edge.NumNodes());
  ASSERT_EQ(1, one_edge.NumEdges());
  const LabeledGraph& input_graph = *one_edge.GetGraph();
  // If no edges are deleted, the graph should not change.
  std::unique_ptr<Morphism> morphism1 = DeleteEdgesNotNodes(input_graph, {});
  EXPECT_TRUE(morphism1->HasOutputGraph());
  const LabeledGraph& graph1 = morphism1->Output();
  EXPECT_EQ(2, graph1.NumNodes());
  EXPECT_EQ(1, graph1.NumEdges());
  // If the edge (0 -> 1) is deleted, there will be two nodes and zero edges
  // left in the graph.
  EdgeId edge = *input_graph.EdgeSetBegin();
  std::unique_ptr<Morphism> morphism2 =
      DeleteEdgesNotNodes(input_graph, {edge});
  EXPECT_TRUE(morphism2->HasOutputGraph());
  const LabeledGraph& graph2 = morphism2->Output();
  EXPECT_EQ(2, graph2.NumNodes());
  EXPECT_EQ(0, graph2.NumEdges());
}

TEST(GraphTransformerTest, DeleteSingleEdgeAndNodes) {
  // Create the graph { 0 -> 1 } and test deletion of edges and disconnected
  // nodes.
  test::WeightedGraph one_edge;
  test::GetPathGraph(2, &one_edge);
  ASSERT_EQ(2, one_edge.NumNodes());
  ASSERT_EQ(1, one_edge.NumEdges());
  const LabeledGraph& input_graph = *one_edge.GetGraph();
  // If no edges are deleted, the graph should not change.
  std::unique_ptr<Morphism> morphism1 = DeleteEdgesAndNodes(input_graph, {});
  EXPECT_TRUE(morphism1->HasOutputGraph());
  const LabeledGraph& graph1 = morphism1->Output();
  EXPECT_EQ(2, graph1.NumNodes());
  EXPECT_EQ(1, graph1.NumEdges());
  // If the edge (0 -> 1) is deleted, there will be zero nodes and edges left.
  EdgeId edge = *input_graph.EdgeSetBegin();
  std::unique_ptr<Morphism> morphism2 =
      DeleteEdgesAndNodes(input_graph, {edge});
  EXPECT_TRUE(morphism2->HasOutputGraph());
  const LabeledGraph& graph2 = morphism2->Output();
  EXPECT_EQ(0, graph2.NumNodes());
  EXPECT_EQ(0, graph2.NumEdges());
}

TEST(GraphTransformerTest, DeleteOnlyEdgeFromCycle) {
  // Create the graph { 0 -> 1, 1 -> 2, 2 -> 0 }.
  test::WeightedGraph triangle;
  test::GetCycleGraph(3, &triangle);
  ASSERT_EQ(3, triangle.NumNodes());
  ASSERT_EQ(3, triangle.NumEdges());
  const LabeledGraph& input_graph = *triangle.GetGraph();
  EdgeIterator edge_it = input_graph.EdgeSetBegin();
  EdgeId edge = *(++edge_it);
  // Check that deleting '1 -> 2' results in the graph { 0 -> 1, 2 -> 0}.
  std::unique_ptr<Morphism> morphism1 =
      DeleteEdgesNotNodes(input_graph, {edge});
  const LabeledGraph& graph1 = morphism1->Output();
  // Check that the graph has two nodes and one edge.
  EXPECT_EQ(3, graph1.NumNodes());
  EXPECT_EQ(2, graph1.NumEdges());
}

TEST(GraphTransformerTest, DeleteEdgeAndNodesFromCycle) {
  // Create the graph { 0 -> 1, 1 -> 2, 2 -> 3, 3 -> 0 }.
  test::WeightedGraph square;
  test::GetCycleGraph(4, &square);
  ASSERT_EQ(4, square.NumNodes());
  ASSERT_EQ(4, square.NumEdges());
  const LabeledGraph& input_graph = *square.GetGraph();
  EdgeIterator edge_it = input_graph.EdgeSetBegin();
  EdgeId edge12 = *(++edge_it);
  EdgeId edge23 = *(++edge_it);
  // Deleting '1 -> 2' and '2 -> 3' should result in the graph { 3 -> 0 -> 1 }.
  std::unique_ptr<Morphism> morphism1 =
      DeleteEdgesAndNodes(input_graph, {edge12, edge23});
  const LabeledGraph& graph1 = morphism1->Output();
  // Check that the graph has two nodes and one edge.
  EXPECT_EQ(3, graph1.NumNodes());
  EXPECT_EQ(2, graph1.NumEdges());
}

// Label functions for QuotientGraph. All of these functions are required to be
// NodeLabelFn's or EdgeLabelFn's so they can be passed as arguments to
// QuotientGraph. Some arguments may not be used by these methods.
TaggedAST ConcatLabels(const LabeledGraph& graph,
                       const std::set<NodeId>& nodes) {
  ast::PrintConfig config;
  auto end_it = nodes.end();
  string label = "{ ";
  auto node_it = nodes.begin();
  util::StrAppend(&label, ast::ToString(graph.GetNodeLabel(*node_it), config));
  for (++node_it; node_it != end_it; ++node_it) {
    util::StrAppend(&label, ", ",
                    ast::ToString(graph.GetNodeLabel(*node_it), config));
  }
  util::StrAppend(&label, " }");
  TaggedAST tagged_label;
  *tagged_label.mutable_ast() = ast::value::MakeString(label);
  tagged_label.set_tag(kBlockTag);
  return tagged_label;
}

std::vector<TaggedAST> EdgeCountLabel(const LabeledGraph& graph,
                                      const std::set<EdgeId>& edges) {
  TaggedAST tagged_label;
  *tagged_label.mutable_ast() = ast::value::MakeInt(edges.size());
  tagged_label.set_tag(kEdgeTag);
  std::vector<TaggedAST> labels;
  labels.push_back(tagged_label);
  return labels;
}

std::vector<TaggedAST> CopyEdgesLabel(const LabeledGraph& graph,
                                     const std::set<EdgeId>& edges) {
  std::vector<TaggedAST> labels;
  for (auto edge : edges) {
    labels.push_back(graph.GetEdgeLabel(edge));
  }
  return labels;
}

TaggedAST LowestIdLabel(const LabeledGraph& graph,
                        const std::set<NodeId>& nodes) {
  ast::PrintConfig config;
  auto node_it = nodes.begin();
  NodeId low = *node_it;
  int val = static_cast<int>(low);
  TaggedAST tagged_label;
  *tagged_label.mutable_ast() = ast::value::MakeInt(val);
  tagged_label.set_tag(kBlockTag);
  return tagged_label;
}

TaggedAST LowestIdWeightedGraphLabel(const LabeledGraph& graph,
                                     const std::set<NodeId>& nodes) {
  ast::PrintConfig config;
  auto node_it = nodes.begin();
  NodeId low = *node_it;
  int val = static_cast<int>(low);
  TaggedAST tagged_label;
  *tagged_label.mutable_ast() = ast::value::MakeInt(val);
  tagged_label.set_tag("Node-Weight");
  return tagged_label;
}

std::vector<TaggedAST> ConstantNodeFoldingLabel(const LabeledGraph& graph,
                                                NodeId node, NodeId predecessor,
                                                NodeId successor) {
  TaggedAST tagged_label;
  *tagged_label.mutable_ast() = ast::value::MakeInt(1);
  tagged_label.set_tag("Edge-Weight");
  std::vector<TaggedAST> labels;
  labels.push_back(tagged_label);
  return labels;
}

std::vector<TaggedAST> EmptyNodeFoldingLabel(const LabeledGraph& graph,
                                             NodeId node, NodeId predecessor,
                                             NodeId successor) {
  std::vector<TaggedAST> labels;
  return labels;
}

std::vector<TaggedAST> DoubleIndicatorNodeFoldingLabel(
    const LabeledGraph& graph, NodeId node,
    NodeId predecessor, NodeId successor) {
  TaggedAST tagged_label_1;
  *tagged_label_1.mutable_ast() = ast::value::MakeInt(1);
  tagged_label_1.set_tag("Edge-Weight");
  TaggedAST tagged_label_2;
  *tagged_label_2.mutable_ast() = ast::value::MakeInt(2);
  tagged_label_2.set_tag("Edge-Weight");
  std::vector<TaggedAST> labels;
  labels.push_back(tagged_label_1);
  labels.push_back(tagged_label_2);
  return labels;
}

// Initialization functions for QuotientGraph tests.
// Initialize 'graph' with strings for node and edge labels.
void SetStringTypes(LabeledGraph* graph) {
  AST node_type = type::MakeString("Label", false);
  type::Types node_types;
  node_types.insert({kBlockTag, node_type});
  AST edge_type = type::MakeString("Label", false);
  type::Types edge_types;
  edge_types.insert({kEdgeTag, edge_type});
  AST graph_type = type::MakeNull("SetStringTypes");
  graph->Initialize(node_types, {}, edge_types, {}, graph_type);
}

// Initialize 'graph' with strings for node labels and integers for edge labels.
void SetStringNodeIntEdgeType(LabeledGraph* graph) {
  AST node_type = type::MakeString("Label", false);
  type::Types node_types;
  node_types.insert({kBlockTag, node_type});
  AST edge_type = type::MakeInt("Label", false);
  type::Types edge_types;
  edge_types.insert({kEdgeTag, edge_type});
  AST graph_type = type::MakeNull("SetStringTypes");
  graph->Initialize(node_types, {}, edge_types, {}, graph_type);
}

// Initialize 'graph' with integers for node labels and strings for edge labels.
void SetIntNodeStringEdgeGraph(LabeledGraph* graph) {
  AST node_type = type::MakeInt("Label", false);
  type::Types node_types;
  node_types.insert({kBlockTag, node_type});
  AST edge_type = type::MakeString("Label", false);
  type::Types edge_types;
  edge_types.insert({kEdgeTag, edge_type});
  AST graph_type = type::MakeNull("SetStringTypes");
  graph->Initialize(node_types, {}, edge_types, {}, graph_type);
}

// Initialize 'graph' with integers for node and edge labels.
void SetIntTypes(LabeledGraph* graph) {
  AST node_type = type::MakeInt("Label", false);
  type::Types node_types;
  node_types.insert({kBlockTag, node_type});
  AST edge_type = type::MakeInt("Label", false);
  type::Types edge_types;
  edge_types.insert({kEdgeTag, edge_type});
  AST graph_type = type::MakeNull("SetIntTypes");
  graph->Initialize(node_types, {}, edge_types, {}, graph_type);
}

// The quotient of a graph with respect to the identity equivalence relation
// should be the same graph.
TEST(GraphTransformerTest, IdentityPathQuotient) {
  // Create the graph { 0 -> 1 } and obtain the identifiers for the two nodes in
  // the graph.
  test::WeightedGraph path1;
  test::GetPathGraph(2, &path1);
  ASSERT_EQ(2, path1.NumNodes());
  ASSERT_EQ(1, path1.NumEdges());
  const LabeledGraph& input_graph1 = *path1.GetGraph();
  NodeIterator node_it = input_graph1.NodeSetBegin();
  NodeId first_node = *node_it;
  ++node_it;
  NodeId second_node = *node_it;

  // Define the identity partition on two nodes.
  std::map<NodeId, int> partition1 {
    {first_node, 1},
    {second_node, 2},
  };

  // Check that the quotient of the graph {0 -> 1} is the same graph.
  LabeledGraph graphtype;
  SetStringNodeIntEdgeType(&graphtype);
  QuotientConfig config1(graphtype, ConcatLabels,
                                EdgeCountLabel, true);
  std::unique_ptr<LabeledGraph> graph1 = QuotientGraph(
      input_graph1, partition1, config1);

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(2, graph1->NumNodes());
  EXPECT_EQ(1, graph1->NumEdges());
  EXPECT_TRUE(test::IsPath(*graph1));

  // Check the same for a path graph with 6 nodes and 5 edges.
  test::WeightedGraph path2;
  test::GetPathGraph(6, &path2);
  ASSERT_EQ(6, path2.NumNodes());
  ASSERT_EQ(5, path2.NumEdges());
  const LabeledGraph& input_graph2 = *path2.GetGraph();
  std::map<NodeId, int> partition2;
  NodeIterator end_it = input_graph2.NodeSetEnd();

  // Define the identity partition on six nodes.
  int i = 1;
  for (node_it = input_graph2.NodeSetBegin(); node_it != end_it; ++node_it) {
    partition2[*node_it] = i;
    ++i;
  }

  // Check the quotient of this longer path is the same graph.
  QuotientConfig config2(graphtype, ConcatLabels,
                                EdgeCountLabel, true);
  std::unique_ptr<LabeledGraph> graph2 = QuotientGraph(
      input_graph2, partition2, config2);

  EXPECT_TRUE(graph2 != nullptr);
  EXPECT_EQ(6, graph2->NumNodes());
  EXPECT_EQ(5, graph2->NumEdges());
  EXPECT_TRUE(test::IsPath(*graph2));
}

TEST(GraphTransformerTest, IdentityCycleQuotient) {
  // Create the graph { 0 <-> 1 } and obtain the identifiers for the two nodes
  // in the graph.
  test::WeightedGraph cycle1;
  test::GetCycleGraph(2, &cycle1);
  ASSERT_EQ(2, cycle1.NumNodes());
  ASSERT_EQ(2, cycle1.NumEdges());
  const LabeledGraph& input_graph1 = *cycle1.GetGraph();
  NodeIterator node_it = input_graph1.NodeSetBegin();
  NodeId first_node = *node_it;
  ++node_it;
  NodeId second_node = *node_it;

  // Make partition splitting graph into two parts
  std::map<NodeId, int> partition1 {
    {first_node, 1},
    {second_node, 2},
  };

  // Check that the quotient of the graph {0 <-> 1} is the same graph.
  LabeledGraph graphtype;
  SetStringNodeIntEdgeType(&graphtype);
  QuotientConfig config1(graphtype, ConcatLabels,
                                EdgeCountLabel, true);
  std::unique_ptr<LabeledGraph> graph1 = QuotientGraph(
      input_graph1, partition1, config1);

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(2, graph1->NumNodes());
  EXPECT_EQ(2, graph1->NumEdges());
  EXPECT_TRUE(test::IsCycle(*graph1));

  // Check the same on a cycle with 6 nodes and edges.
  test::WeightedGraph cycle2;
  test::GetCycleGraph(6, &cycle2);
  ASSERT_EQ(6, cycle2.NumNodes());
  ASSERT_EQ(6, cycle2.NumEdges());
  const LabeledGraph& input_graph2 = *cycle2.GetGraph();

  std::map<NodeId, int> partition2;
  NodeIterator end_it = input_graph2.NodeSetEnd();

  // Make identity partiton on this cycle.
  int i = 1;
  for (node_it = input_graph2.NodeSetBegin(); node_it != end_it; ++node_it) {
    partition2[*node_it] = i;
    ++i;
  }

  // Check the identity partition preserves the graph under graph quotients.
  QuotientConfig config2(graphtype, ConcatLabels,
                                EdgeCountLabel, true);
  std::unique_ptr<LabeledGraph> graph2 = QuotientGraph(
      input_graph2, partition2, config2);

  EXPECT_TRUE(graph2 != nullptr);
  EXPECT_EQ(6, graph2->NumNodes());
  EXPECT_EQ(6, graph2->NumEdges());
  EXPECT_TRUE(test::IsCycle(*graph2));
}

TEST(GraphTransformerTest, SimplePathQuotient) {
  // Create the graph { 0 -> 1 } and obtain the identifiers for the two nodes
  // in the graph.
  test::WeightedGraph path1;
  test::GetPathGraph(2, &path1);
  ASSERT_EQ(2, path1.NumNodes());
  ASSERT_EQ(1, path1.NumEdges());
  const LabeledGraph& input_graph = *path1.GetGraph();
  NodeIterator node_it = input_graph.NodeSetBegin();
  NodeId first_node = *node_it;
  ++node_it;
  NodeId second_node = *node_it;

  // Make partition with a single block.
  std::map<NodeId, int> partition1 {
    {first_node, 1},
    {second_node, 1},
  };

  // Check that the quotient of the graph {0 -> 1} is one node with a single
  // self-edge.
  LabeledGraph graphtype;
  SetStringNodeIntEdgeType(&graphtype);
  QuotientConfig config(graphtype, ConcatLabels,
                               EdgeCountLabel, true);
  std::unique_ptr<LabeledGraph> graph1 = QuotientGraph(
      input_graph, partition1, config);

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(1, graph1->NumNodes());
  EXPECT_EQ(1, graph1->NumEdges());
  EXPECT_FALSE(test::IsPath(*graph1));
  EXPECT_TRUE(test::IsCycle(*graph1));

  // Check that the label records the original edge multiplicity.
  EdgeIterator edge_it = graph1->EdgeSetBegin();
  TaggedAST edge_label = graph1->GetEdgeLabel(*edge_it);
  EXPECT_EQ(1, edge_label.ast().p_ast().val().int_val());
}

TEST(GraphTransformerTest, SimpleCycleQuotient) {
  // Create the graph { 0 <-> 1 } and obtain the identifiers for the two nodes
  // in the graph.
  test::WeightedGraph cycle1;
  test::GetCycleGraph(2, &cycle1);
  ASSERT_EQ(2, cycle1.NumNodes());
  ASSERT_EQ(2, cycle1.NumEdges());
  const LabeledGraph& input_graph = *cycle1.GetGraph();
  NodeIterator node_it = input_graph.NodeSetBegin();
  NodeId first_node = *node_it;
  ++node_it;
  NodeId second_node = *node_it;

  // Make partition with a single block.
  std::map<NodeId, int> partition1 {
    {first_node, 1},
    {second_node, 1},
  };

  // Check that the quotient of the graph {0 <-> 1} is one node with a single
  // self-edge.
  LabeledGraph graphtype;
  SetStringNodeIntEdgeType(&graphtype);
  QuotientConfig config(graphtype, ConcatLabels,
                               EdgeCountLabel, true);
  std::unique_ptr<LabeledGraph> graph1 = QuotientGraph(
      input_graph, partition1, config);

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(1, graph1->NumNodes());
  EXPECT_EQ(1, graph1->NumEdges());
  EXPECT_TRUE(test::IsCycle(*graph1));

  // Check that the label records the original edge multiplicity.
  EdgeIterator edge_it = graph1->EdgeSetBegin();
  TaggedAST edge_label = graph1->GetEdgeLabel(*edge_it);
  EXPECT_EQ(2, edge_label.ast().p_ast().val().int_val());
}

TEST(GraphTransformerTest, MultiEdgeQuotient) {
  // Create the graph { 0 <-> 1 } and obtain the identifiers for the two nodes
  // in the graph.
  test::WeightedGraph cycle1;
  test::GetCycleGraph(2, &cycle1);
  ASSERT_EQ(2, cycle1.NumNodes());
  ASSERT_EQ(2, cycle1.NumEdges());
  const LabeledGraph& input_graph1 = *cycle1.GetGraph();
  NodeIterator node_it = input_graph1.NodeSetBegin();
  NodeId first_node = *node_it;
  ++node_it;
  NodeId second_node = *node_it;

  // Make partition with a single block.
  std::map<NodeId, int> partition1{
      {first_node, 1}, {second_node, 1},
  };

  // Check that the quotient of the graph {0 <-> 1} is one node with a two
  // self-edges.
  QuotientConfig config1(input_graph1, LowestIdWeightedGraphLabel,
                                CopyEdgesLabel, true);
  std::unique_ptr<LabeledGraph> graph1 = QuotientGraph(
      input_graph1, partition1, config1);

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(1, graph1->NumNodes());
  EXPECT_EQ(2, graph1->NumEdges());
  EXPECT_FALSE(test::IsPath(*graph1));
  EXPECT_FALSE(test::IsCycle(*graph1));

  // Check the same on a cycle with 3 nodes and edges.
  test::WeightedGraph cycle2;
  test::GetCycleGraph(3, &cycle2);
  ASSERT_EQ(3, cycle2.NumNodes());
  ASSERT_EQ(3, cycle2.NumEdges());
  const LabeledGraph& input_graph2 = *cycle2.GetGraph();

  std::map<NodeId, int> partition2;
  NodeIterator end_it = input_graph2.NodeSetEnd();

  // Make partition with a single block.
  for (node_it = input_graph2.NodeSetBegin(); node_it != end_it; ++node_it) {
    partition2[*node_it] = 1;
  }

  // Check that the quotient has 1 node with 3 self-edges.
  QuotientConfig config2(input_graph2, LowestIdWeightedGraphLabel,
                                CopyEdgesLabel, true);
  std::unique_ptr<LabeledGraph> graph2 = QuotientGraph(
      input_graph2, partition2, config2);

  EXPECT_TRUE(graph2 != nullptr);
  EXPECT_EQ(1, graph2->NumNodes());
  EXPECT_EQ(3, graph2->NumEdges());
  EXPECT_FALSE(test::IsPath(*graph2));
  EXPECT_FALSE(test::IsCycle(*graph2));
}

TEST(GraphTransformerTest, RelabelQuotient) {
  // Create cycle graph with 6 nodes and edges.
  test::WeightedGraph cycle;
  test::GetCycleGraph(6, &cycle);
  ASSERT_EQ(6, cycle.NumNodes());
  ASSERT_EQ(6, cycle.NumEdges());
  const LabeledGraph& input_graph = *cycle.GetGraph();

  std::map<NodeId, int> partition;
  NodeIterator end_it = input_graph.NodeSetEnd();

  int i = 0;
  // Make partition with two blocks, with the first and last three nodes grouped
  // together.
  // TODO(jefflai): Remove reliance on node id arithmetic.
  for (auto node_it = input_graph.NodeSetBegin(); node_it != end_it;
       ++node_it) {
    partition[*node_it] = i < 3 ? 1 : 2;
    ++i;
  }

  // Check that the resultant graph has 2 nodes, each of which has 1 self-edge.
  // Check also there are two edges going between the nodes, one in each
  // direction.
  LabeledGraph graphtype;
  SetIntTypes(&graphtype);
  QuotientConfig config(graphtype, LowestIdLabel,
                               EdgeCountLabel, true);
  std::unique_ptr<LabeledGraph> graph = QuotientGraph(
      input_graph, partition, config);
  EXPECT_TRUE(graph != nullptr);
  EXPECT_EQ(2, graph->NumNodes());
  EXPECT_EQ(4, graph->NumEdges());
  EXPECT_FALSE(test::IsPath(*graph));
  EXPECT_FALSE(test::IsCycle(*graph));

  // Get the two nodes.
  auto output_node_it = graph->NodeSetBegin();
  NodeId first_node = *output_node_it;
  ++output_node_it;
  NodeId second_node = *output_node_it;

  // Check their labels are the lowest id of each block.
  int first_label =
      graph->GetNodeLabel(first_node).ast().p_ast().val().int_val();
  int second_label =
      graph->GetNodeLabel(second_node).ast().p_ast().val().int_val();
  EXPECT_EQ(0, first_label);
  EXPECT_EQ(3, second_label);

  // Check the weights are correct: self-edges are 2, otherwise 1.
  auto edge_end_it = graph->EdgeSetEnd();
  for (auto edge_it = graph->EdgeSetBegin(); edge_it != edge_end_it;
       ++edge_it) {
    EdgeId edge = *edge_it;
    int weight = graph->GetEdgeLabel(edge).ast().p_ast().val().int_val();
    if (graph->Source(edge) == graph->Target(edge)) {
      EXPECT_EQ(2, weight);
    } else {
      EXPECT_EQ(1, weight);
    }
  }
}

// Takes the graph { 0 <-> 1 } and quotient's it as a single block, disallowing
// self-edges. The result should be a graph with just one node.
TEST(GraphTransformerTest, SimpleNoSelfEdgeQuotient) {
  test::WeightedGraph cycle1;
  test::GetCycleGraph(2, &cycle1);
  ASSERT_EQ(2, cycle1.NumNodes());
  ASSERT_EQ(2, cycle1.NumEdges());
  const LabeledGraph& input_graph = *cycle1.GetGraph();
  NodeIterator node_it = input_graph.NodeSetBegin();
  NodeId first_node = *node_it;
  ++node_it;
  NodeId second_node = *node_it;

  // Make partition with a single block.
  std::map<NodeId, int> partition1 {
    {first_node, 1},
    {second_node, 1},
  };

  // Check that the quotient of the graph is one node without edges.
  LabeledGraph graphtype;
  SetStringNodeIntEdgeType(&graphtype);
  QuotientConfig config(graphtype, ConcatLabels,
                               EdgeCountLabel, false);
  std::unique_ptr<LabeledGraph> graph1 = QuotientGraph(
      input_graph, partition1, config);

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(1, graph1->NumNodes());
  EXPECT_EQ(0, graph1->NumEdges());
  EXPECT_TRUE(test::IsPath(*graph1));
}

// Calls edge contraction on an empty edge-set. The graph should be unchanged
// after the contraction.
TEST(GraphTransformerTest, NoEdgeContraction) {
  // Create the graph { 0 -> 1 -> 2}.
  test::WeightedGraph path;
  test::GetPathGraph(3, &path);
  ASSERT_EQ(3, path.NumNodes());
  ASSERT_EQ(2, path.NumEdges());
  const LabeledGraph& input_graph = *path.GetGraph();

  // Check that contracting no edges results in the same graph.
  LabeledGraph graphtype;
  SetStringNodeIntEdgeType(&graphtype);
  QuotientConfig config(graphtype, ConcatLabels,
                               EdgeCountLabel, true);
  std::unique_ptr<LabeledGraph> graph1 = ContractEdges(
      input_graph, {}, config);

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(3, graph1->NumNodes());
  EXPECT_EQ(2, graph1->NumEdges());
  EXPECT_TRUE(test::IsPath(*graph1));
}

// Takes the graph with a single node with a single self-edge and contracts that
// self-edge. The result should be just the one node.
TEST(GraphTransformerTest, SelfLoopEdgeContraction) {
  test::WeightedGraph cycle;
  test::GetCycleGraph(1, &cycle);
  ASSERT_EQ(1, cycle.NumNodes());
  ASSERT_EQ(1, cycle.NumEdges());
  const LabeledGraph& input_graph = *cycle.GetGraph();
  auto edge_it = input_graph.EdgeSetBegin();
  auto edge = *edge_it;

  LabeledGraph graphtype;
  SetStringNodeIntEdgeType(&graphtype);
  QuotientConfig config(graphtype, ConcatLabels,
                               EdgeCountLabel, true);
  std::unique_ptr<LabeledGraph> graph1 = ContractEdges(
      input_graph, {edge}, config);

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(1, graph1->NumNodes());
  EXPECT_EQ(0, graph1->NumEdges());
  EXPECT_TRUE(test::IsPath(*graph1));
}

// Contracts the one edge in the graph { 0 -> 1}. Should result in a graph with
// one node and no edges.
TEST(GraphTransformerTest, SimplePathEdgeContraction) {
  test::WeightedGraph path;
  test::GetPathGraph(2, &path);
  ASSERT_EQ(2, path.NumNodes());
  ASSERT_EQ(1, path.NumEdges());
  const LabeledGraph& input_graph = *path.GetGraph();
  auto edge_it = input_graph.EdgeSetBegin();
  auto edge = *edge_it;

  LabeledGraph graphtype;
  SetStringNodeIntEdgeType(&graphtype);
  QuotientConfig config(graphtype, ConcatLabels,
                               EdgeCountLabel, true);
  std::unique_ptr<LabeledGraph> graph1 = ContractEdges(
      input_graph, {edge}, config);

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(1, graph1->NumNodes());
  EXPECT_EQ(0, graph1->NumEdges());
  EXPECT_TRUE(test::IsPath(*graph1));
}

// Contracts one of the edges in the graph { 0 <-> 1}. Should result in a graph
// with a one node and one self-edge.
TEST(GraphTransformerTest, SimpleCycleEdgeContraction) {
  test::WeightedGraph cycle;
  test::GetCycleGraph(2, &cycle);
  ASSERT_EQ(2, cycle.NumNodes());
  ASSERT_EQ(2, cycle.NumEdges());
  const LabeledGraph& input_graph = *cycle.GetGraph();
  auto edge_it = input_graph.EdgeSetBegin();
  auto edge = *edge_it;

  LabeledGraph graphtype;
  SetStringNodeIntEdgeType(&graphtype);
  QuotientConfig config(graphtype, ConcatLabels,
                               EdgeCountLabel, true);
  std::unique_ptr<LabeledGraph> graph1 = ContractEdges(
      input_graph, {edge}, config);

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(1, graph1->NumNodes());
  EXPECT_EQ(1, graph1->NumEdges());
  EXPECT_TRUE(test::IsCycle(*graph1));
}

// Takes the graph { 0 -> 1 -> 2 -> 3 -> 4 -> 5} and contracts two edges. The
// result should be a path of length 4.
TEST(GraphTransformerTest, LongPathEdgeContraction) {
  test::WeightedGraph path;
  test::GetPathGraph(6, &path);
  ASSERT_EQ(6, path.NumNodes());
  ASSERT_EQ(5, path.NumEdges());
  const LabeledGraph& input_graph = *path.GetGraph();
  auto edge_it = input_graph.EdgeSetBegin();
  EdgeId edge1 = *edge_it;
  ++edge_it;
  EdgeId edge2 = *edge_it;

  LabeledGraph graphtype;
  SetStringNodeIntEdgeType(&graphtype);
  QuotientConfig config(graphtype, ConcatLabels,
                               EdgeCountLabel, true);
  std::unique_ptr<LabeledGraph> graph1 = ContractEdges(
      input_graph, {edge1, edge2}, config);

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(4, graph1->NumNodes());
  EXPECT_EQ(3, graph1->NumEdges());
  EXPECT_TRUE(test::IsPath(*graph1));
}

TEST(GraphTransformerTest, NoNodeFold) {
  // Create the graph { 0 -> 1 -> 2}.
  test::WeightedGraph path;
  test::GetPathGraph(3, &path);
  ASSERT_EQ(3, path.NumNodes());
  ASSERT_EQ(2, path.NumEdges());
  const LabeledGraph& input_graph = *path.GetGraph();

  // Check that folding no vertices results in the same graph.
  std::unique_ptr<LabeledGraph> graph1 = FoldNodes(
      input_graph, ConstantNodeFoldingLabel, {});

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(3, graph1->NumNodes());
  EXPECT_EQ(2, graph1->NumEdges());
  EXPECT_TRUE(test::IsPath(*graph1));
}

// Takes the graph with a single node with a single self-edge and folds that
// node. The result should be an empty graph.
TEST(GraphTransformerTest, SingleNodeNodeFold) {
  test::WeightedGraph cycle;
  test::GetCycleGraph(1, &cycle);
  ASSERT_EQ(1, cycle.NumNodes());
  ASSERT_EQ(1, cycle.NumEdges());
  const LabeledGraph& input_graph = *cycle.GetGraph();
  auto node_it = input_graph.NodeSetBegin();
  auto node = *node_it;

  std::unique_ptr<LabeledGraph> graph1 = FoldNodes(
      input_graph, ConstantNodeFoldingLabel, {node});

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(0, graph1->NumNodes());
  EXPECT_EQ(0, graph1->NumEdges());
}

// Folds a node in the graph { 0 -> 1}. Should result in a graph with
// one node and no edges.
TEST(GraphTransformerTest, SimplePathNodeFold) {
  test::WeightedGraph path;
  test::GetPathGraph(2, &path);
  ASSERT_EQ(2, path.NumNodes());
  ASSERT_EQ(1, path.NumEdges());
  const LabeledGraph& input_graph = *path.GetGraph();
  auto node_it = input_graph.NodeSetBegin();
  auto node = *node_it;

  std::unique_ptr<LabeledGraph> graph1 = FoldNodes(
      input_graph, ConstantNodeFoldingLabel, {node});

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(1, graph1->NumNodes());
  EXPECT_EQ(0, graph1->NumEdges());
  EXPECT_TRUE(test::IsPath(*graph1));
}

// Folds a node in the graph { 0 <-> 1}. Should result in a graph
// with a one node and one self-edge.
TEST(GraphTransformerTest, SimpleCycleNodeFold) {
  test::WeightedGraph cycle;
  test::GetCycleGraph(2, &cycle);
  ASSERT_EQ(2, cycle.NumNodes());
  ASSERT_EQ(2, cycle.NumEdges());
  const LabeledGraph& input_graph = *cycle.GetGraph();
  auto node_it = input_graph.NodeSetBegin();
  auto node = *node_it;

  std::unique_ptr<LabeledGraph> graph1 = FoldNodes(
      input_graph, ConstantNodeFoldingLabel, {node});

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(1, graph1->NumNodes());
  EXPECT_EQ(1, graph1->NumEdges());
  EXPECT_TRUE(test::IsCycle(*graph1));
}

// Takes the graph { 0 -> 1 -> 2 -> 3 -> 4 -> 5} and folds two nodes. The
// result should be a path of length 4.
TEST(GraphTransformerTest, LongPathNodeFold) {
  test::WeightedGraph path;
  test::GetPathGraph(6, &path);
  ASSERT_EQ(6, path.NumNodes());
  ASSERT_EQ(5, path.NumEdges());
  const LabeledGraph& input_graph = *path.GetGraph();
  auto node_it = input_graph.NodeSetBegin();
  auto node1 = *node_it;
  ++node_it;
  auto node2 = *node_it;

  std::unique_ptr<LabeledGraph> graph1 = FoldNodes(
      input_graph, ConstantNodeFoldingLabel, {node1, node2});

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(4, graph1->NumNodes());
  EXPECT_EQ(3, graph1->NumEdges());
  EXPECT_TRUE(test::IsPath(*graph1));
}

// Folds 1 in the graph { 0 -> 1 -> 2}, with a label function that outputs an
// empty set of labels. The output should be two nodes with no edges.
TEST(GraphTransformerTest, EmptyLabelNodeFold) {
  test::WeightedGraph path;
  test::GetPathGraph(3, &path);
  ASSERT_EQ(3, path.NumNodes());
  ASSERT_EQ(2, path.NumEdges());
  const LabeledGraph& input_graph = *path.GetGraph();
  auto node_it = input_graph.NodeSetBegin();
  node_it++;
  auto node = *node_it;

  std::unique_ptr<LabeledGraph> graph1 = FoldNodes(
      input_graph, EmptyNodeFoldingLabel, {node});

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(2, graph1->NumNodes());
  EXPECT_EQ(0, graph1->NumEdges());
}

// Folds 1 in the graph { 0 -> 1 -> 2}, with a label function that outputs a
// set of two labels. The output should be two nodes with two edges.
TEST(GraphTransformerTest, DoubleIndicatorLabelNodeFold) {
  test::WeightedGraph path;
  test::GetPathGraph(3, &path);
  ASSERT_EQ(3, path.NumNodes());
  ASSERT_EQ(2, path.NumEdges());
  const LabeledGraph& input_graph = *path.GetGraph();
  auto node_it = input_graph.NodeSetBegin();
  node_it++;
  auto node = *node_it;

  std::unique_ptr<LabeledGraph> graph1 = FoldNodes(
      input_graph, DoubleIndicatorNodeFoldingLabel, {node});

  EXPECT_TRUE(graph1 != nullptr);
  EXPECT_EQ(2, graph1->NumNodes());
  EXPECT_EQ(2, graph1->NumEdges());
}

}  // namespace
}  // namespace graph
}  // namespace morphie
