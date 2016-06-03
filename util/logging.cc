#include "util/logging.h"

#include <cassert>   // for assert()
#include <cstdlib>   // for abort()
#include <iostream>  // for std::cerr


namespace logle {
namespace util {

void Check(bool condition, const string& location, const string& err) {
  if (!condition) {
    std::cerr << location << ": " << err;
    std::abort();
  }
}

void Check(bool condition, const string& location) {
  Check(condition, location, "");
}

void Check(bool condition) { Check(condition, "", ""); }

}  // namespace util
}  // namespace logle
