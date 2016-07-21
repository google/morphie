#include <iostream>
#include "stream_dependency_graph.h"

int main(int argc, char **argv) {
  logle::StreamDependencyGraph graph;
  graph.Initialize();
  std::cout << "Initialized account access graph." << std::endl
            << graph.ToDot() << std::endl;
}
