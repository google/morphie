#include <iostream>
#include "ast.pb.h"
#include "type.h"
#include "type_checker.h"

int main(int argc, char **argv) {
  logle::AST ast = logle::ast::type::MakeBool("ast", false);
  std::cout << ast.DebugString();
  std::string err;
  if (logle::ast::type::IsType(ast, &err)) {
    std::cout <<  " is a type." << std::endl;
  } else {
    std::cout <<  " is not a type." << std::endl;
  }
}
