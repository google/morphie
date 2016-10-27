#include "graph_exporter.h"

#include <iostream>

#include "ast.pb.h"
#include "labeled_graph.h"
#include "type.h"

int main(int argc, char **argv) {
  tervuren::LabeledGraph graph;
  tervuren::AST ast = tervuren::ast::type::MakeInt("int label", false);
  graph.Initialize({}, {}, {}, {}, ast);
  tervuren::viz::GraphExporter exporter(graph);
  std::cout << "Exported graph: " << exporter.GraphAsString() << std::endl;
}
