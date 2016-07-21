#include <iostream>

#include "util/logging.h"

int main(int argc, char **argv) {
  logle::util::Check(true);
  std::cout << "Check test passed." << std::endl;
  return 0;
}
