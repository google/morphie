#include <iostream>
#include <string>

#include "third_party/logle/graph/ast.pb.h"
#include "type_checker.h"

int main(int argc, char **argv) {
  morphie::AST ast;
  ast.set_name("name");
  ast.set_is_nullable("false");
  std::cout << ast.DebugString();
  std::string err;
  if (morphie::ast::type::IsType(ast, &err)) {
    std::cout << std::endl << "is a type." << std::endl;
  } else {
    std::cout << std::endl << "is not a type." << std::endl;
  }
  return 0;
}
