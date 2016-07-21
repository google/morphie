#include <iostream>

#include "ast.pb.h"

int main(int argc, char **argv) {
  logle::AST ast;
  logle::PrimitiveValue pval;
  pval.set_int_val(5);
  ast.mutable_p_ast()->set_type(logle::INT);
  *(ast.mutable_p_ast()->mutable_val()) = pval;
  std::cout << ast.DebugString() << std::endl;
  /*
  std::cout << "Hello World" << std::endl;
  */
  return 0;
}
