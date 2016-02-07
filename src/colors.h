#pragma once

#include <string>

namespace Color {

void init_colors();
void free_colors();

size_t color_max();
std::string const& color_get(size_t i);
std::string const& color_random();

}
