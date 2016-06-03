// This file defines a function for checking assertions and printing an error
// if
// the assertion fails.
#ifndef LOGLE_UTIL_LOGGING_H_
#define LOGLE_UTIL_LOGGING_H_

#include "base/string.h"

#define MAKE_STR(x) #x
#define TOSTRING(x) MAKE_STR(x)
#define LOCATION_STR __FILE__ ":" TOSTRING(__LINE__)
#define CHECK(c, err) util::Check(c, LOCATION_STR, err)

namespace logle {
namespace util {

// Produces an error message and aborts if the condition is false.
void Check(bool condition, const string& location, const string& err);
void Check(bool condition, const string& location);
void Check(bool condition);

}  // namespace util
}  // logle

#endif  // LOGLE_UTIL_LOGGING_H_
