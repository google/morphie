#include <iostream>
#include "ast.pb.h"
#include "dot_printer.h"
#include "graph_transformer.h"
#include "labeled_graph.h"
#include "type.h"
#include "value.h"

int main(int argc, char **argv) {
  logle::LabeledGraph graph;
  logle::AST ast = logle::ast::type::MakeInt("int label", false);
  logle::TaggedAST tast;
  tast.set_tag("num");
  *tast.mutable_ast() = logle::ast::value::MakeInt(0);
  graph.Initialize({{"num", ast}}, {}, {}, {}, ast);
  logle::NodeId node0 = graph.FindOrAddNode(tast);
  *tast.mutable_ast() = logle::ast::value::MakeInt(1);
  graph.FindOrAddNode(tast);
  logle::DotPrinter printer;
  std::cout << "Input graph." << std::endl
            << printer.DotGraph(graph) << std::endl
            << "Output graph." << std::endl
            << printer.DotGraph(*logle::graph::DeleteNodes(graph, {node0}))
            << std::endl;
}
