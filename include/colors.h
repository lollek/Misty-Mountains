#pragma once

#include <string>

namespace Color {

void init_colors();
void free_colors();

size_t max();
std::string const& get(size_t i);
std::string const& random();
}
