#include <string>
#include <vector>

#include "os.h"

#include "colors.h"

using namespace std;

static vector<string const>* rainbow = nullptr;

void Color::init_colors() {
  rainbow = new vector<string const> {
    "amber",     "aquamarine", "black",      "blue",       "brown",
    "clear",     "crimson",    "cyan",       "ecru",       "gold",
    "green",     "grey",       "magenta",    "orange",     "pink",
    "plaid",     "purple",     "red",        "silver",     "tan",
    "tangerine", "topaz",      "turquoise",  "vermilion",  "violet",
    "white",     "yellow",
  };
}

void Color::free_colors() {
  delete rainbow;
  rainbow = nullptr;
}

size_t Color::max() {
  return rainbow->size();
}

string const& Color::get(size_t i) {
  return rainbow->at(i);
}

string const& Color::random() {
  return rainbow->at(os_rand_range(rainbow->size()));
}
