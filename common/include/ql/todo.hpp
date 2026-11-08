#pragma once
// Marker for code you still have to write.  A test that hits it fails with a
// readable message instead of crashing.  Delete the call when you implement it.

#include <stdexcept>
#include <string>

namespace ql {

[[noreturn]] inline void todo(const char* what) {
  throw std::logic_error(std::string("TODO: implement ") + what);
}

}  // namespace ql
