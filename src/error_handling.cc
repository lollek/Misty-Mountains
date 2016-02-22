#include <sstream>

#include "error_handling.h"

fatal_error::fatal_error(const std::string &arg, const char *file, int line) :
  std::runtime_error(arg) {
    std::ostringstream o;
    o << file << ":" << line << ": " << arg;
    msg = o.str();
  }

const char *fatal_error::what() const noexcept {
  return msg.c_str();
}

