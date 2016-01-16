#include <string>
#include <vector>

using namespace std;

#include "os.h"

#include "colors.h"

static vector<string const> rainbow = {
    "amber",     "aquamarine", "black",      "blue",       "brown",
    "clear",     "crimson",    "cyan",       "ecru",       "gold",
    "green",     "grey",       "magenta",    "orange",     "pink",
    "plaid",     "purple",     "red",        "silver",     "tan",
    "tangerine", "topaz",      "turquoise",  "vermilion",  "violet",
    "white",     "yellow",
};

size_t
color_max()
{
  return rainbow.size();
}

string const&
color_get(size_t i)
{
  return rainbow.at(i);
}

string const&
color_random()
{
  return rainbow.at(os_rand_range(rainbow.size()));
}
