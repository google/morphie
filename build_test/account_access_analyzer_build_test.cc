#include <iostream>
#include <memory>

#include "account_access_analyzer.h"

int main(int argc, char **argv) {
  logle::AccessAnalyzer analyzer;
  std::unique_ptr<logle::util::CSVParser> parser(
      new logle::util::CSVParser(new std::stringstream("")));
  analyzer.Initialize(std::move(parser));
  std::cout << "Initialized account access analyzer." << std::endl;
}
