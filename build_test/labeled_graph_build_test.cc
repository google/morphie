#nclude <iostream>
#include "ast.pb.h"
#include "labeled_graph.h"
#include "type.h"

int main(int argc, char **argv) {
  logle::LabeledGraph graph;
  logle::AST ast = logle::ast::type::MakeInt("int label", false);
  graph.Initialize({}, {}, {}, {}, ast);
  std::cout << "Initialized graph." << std::endl;
}
