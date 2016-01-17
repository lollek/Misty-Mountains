#pragma once

// From http://stackoverflow.com/a/348862

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

class fatal_error : public std::runtime_error {
    std::string msg;
public:
    fatal_error(const std::string &arg, const char *file, int line);
    const char *what() const noexcept;
};

#define error(arg) throw fatal_error(arg, __FILE__, __LINE__);
