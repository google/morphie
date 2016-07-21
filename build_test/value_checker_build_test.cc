#include <iostream>

#include "ast.pb.h"
#include "value_checker.h"


int main(int argc, char **argv) {
  logle::AST ast;
  std::cout << ast.DebugString();
  std::string err;
  std::cout << ast.DebugString();
  if (logle::ast::value::IsValue(ast, &err)) {
    std::cout << std::endl << " is a value." << std::endl;
  } else {
    std::cout << std::endl << " is not a value." << std::endl;
  }
  return 0;
}
