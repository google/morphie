#include <iostream>

#include "frontend.h"


int main(int argc, char** argv) {

  logle::frontend::Analyzer a = logle::frontend::Analyzer::kCurio;
  logle::AnalysisOptions options;
  options.set_json_file("/usr/local/google/home/vijaydsilva/curio/one-stream");
  options.set_output_dot_file("/tmp/test_bin_output.dot");

  std::cout << options.DebugString() << std::endl;
  if (!logle::frontend::Run(a, options).ok())
    std::cout << "FAILED" << std::endl;
  else
    std::cout << "PASSED" << std::endl;

  return 0;
}
