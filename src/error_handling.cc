#include <sstream>

#include "error_handling.h"

fatal_error::fatal_error(std::string const& arg, char const* file, int line)
    : std::runtime_error(arg) {
  std::ostringstream o;
  o << file << ":" << line << ": " << arg;
  msg = o.str();
}

char const*fatal_error::what() const noexcept { return msg.c_str(); }
