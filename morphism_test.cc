#include "morphism.h"

#include "gtest.h"
#include "test_graphs.h"

namespace tervuren {
namespace graph {
namespace {

TEST(MorphismTest, ConstructorDoesNotCreateOutputGraph) {
  test::WeightedGraph weighted_graph;
  test::GetPathGraph(2, &weighted_graph);
  Morphism morphism(weighted_graph.GetGraph());
  EXPECT_FALSE(morphism.HasOutputGraph());
}

TEST(MorphismTest, CloneGraphCreatesEmptyOutputGraph) {
  test::WeightedGraph weighted_graph;
  test::GetPathGraph(2, &weighted_graph);
  Morphism morphism(weighted_graph.GetGraph());
  morphism.CloneInputType();
  EXPECT_TRUE(morphism.HasOutputGraph());
  EXPECT_EQ(0, morphism.Output().NumNodes());
}

}  // namespace
}  // namespace graph
}  // namespace tervuren
