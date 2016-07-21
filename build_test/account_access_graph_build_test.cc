#include <iostream>
#include "account_access_graph.h"

int main(int argc, char **argv) {
  logle::AccountAccessGraph graph;
  graph.Initialize();
  std::cout << "Initialized account access graph." << std::endl
            << graph.ToDot() << std::endl;
}
