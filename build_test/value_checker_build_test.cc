#include <iostream>

#include "third_party/logle/graph/ast.pb.h"
#include "value_checker.h"


int main(int argc, char **argv) {
  morphie::AST ast;
  std::cout << ast.DebugString();
  std::string err;
  std::cout << ast.DebugString();
  if (morphie::ast::value::IsValue(ast, &err)) {
    std::cout << std::endl << " is a value." << std::endl;
  } else {
    std::cout << std::endl << " is not a value." << std::endl;
  }
  return 0;
}
