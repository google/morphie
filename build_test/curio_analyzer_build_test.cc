#include <iostream>

#include "curio_analyzer.h"
#include "json/json.h"

int main(int argc, char **argv) {
  std::unique_ptr<::Json::Value> doc(new ::Json::Value);
  logle::CurioAnalyzer analyzer;
  analyzer.Initialize(std::move(doc));
  std::cout << "Initialized Curio analyzer." << std::endl;
}
