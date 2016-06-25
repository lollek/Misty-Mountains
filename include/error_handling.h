#pragma once

// From http://stackoverflow.com/a/348862

#include <stdexcept>
#include <string>

class fatal_error : public std::runtime_error {
  std::string msg;

 public:
  fatal_error(std::string const& arg, char const* file, int line);
  char const* what() const noexcept;
};

#define error(arg) throw fatal_error(arg, __FILE__, __LINE__);
