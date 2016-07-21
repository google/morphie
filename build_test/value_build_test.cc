#include <iostream>
#include <string>

#include "ast.pb.h"
#include "value.h"
#include "value_checker.h"

int main(int argc, char **argv) {
  logle::AST ast = logle::ast::value::MakeBool(false);
  std::string err;
  std::cout << ast.DebugString();
  if (logle::ast::value::IsValue(ast, &err)) {
    std::cout <<  " is a value." << std::endl;
  } else {
    std::cout <<  " is not a value." << std::endl;
  }
}
