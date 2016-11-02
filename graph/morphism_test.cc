#include "morphism.h"

#include "gtest.h"
#include "test_graphs.h"
#include "value.h"

namespace morphie {
namespace graph {
namespace {

const char kNodeWeightTag[] = "Node-Weight";
const char kEdgeWeightTag[] = "Edge-Weight";

TEST(MorphismTest, ConstructorDoesNotCreateOutputGraph) {
  test::WeightedGraph weighted_graph;
  test::GetPathGraph(2, &weighted_graph);
  Morphism morphism(weighted_graph.GetGraph());
  EXPECT_FALSE(morphism.HasOutputGraph());
}

TEST(MorphismTest, CopyGraphCreatesEmptyOutputGraph) {
  test::WeightedGraph weighted_graph;
  test::GetPathGraph(2, &weighted_graph);
  Morphism morphism(weighted_graph.GetGraph());
  morphism.CopyInputType();
  EXPECT_TRUE(morphism.HasOutputGraph());
  EXPECT_EQ(0, morphism.Output().NumNodes());
}

TEST(MorphismTest, OutputGraphProperties) {
  test::WeightedGraph weighted_graph;
  test::GetPathGraph(2, &weighted_graph);
  Morphism morphism(weighted_graph.GetGraph());
  morphism.CopyInputType();
  // Add a node to the output graph.
  TaggedAST label;
  label.set_tag(kNodeWeightTag);
  *label.mutable_ast() = ast::value::MakeInt(2);
  morphism.MutableOutput()->FindOrAddNode(label);
  EXPECT_EQ(1, morphism.Output().NumNodes());
}

TEST(MorphismTest, TakeOutputProperties) {
  test::WeightedGraph weighted_graph;
  test::GetPathGraph(2, &weighted_graph);
  Morphism morphism(weighted_graph.GetGraph());
  morphism.CopyInputType();
  ASSERT_TRUE(morphism.HasOutputGraph());
  auto output_graph = morphism.TakeOutput();
  EXPECT_FALSE(morphism.HasOutputGraph());
}

}  // namespace
}  // namespace graph
}  // namespace morphie
