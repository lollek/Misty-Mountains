#include <stdlib.h>
#include <stdbool.h>

#include "potions.h"
#include "os.h"
#include "rogue.h"

#include "colors.h"

static char const* rainbow[] = {
    "amber",     "aquamarine", "black",      "blue",       "brown",
    "clear",     "crimson",    "cyan",       "ecru",       "gold",
    "green",     "grey",       "magenta",    "orange",     "pink",
    "plaid",     "purple",     "red",        "silver",     "tan",
    "tangerine", "topaz",      "turquoise",  "vermilion",  "violet",
    "white",     "yellow",
};

#define NCOLORS (sizeof(rainbow) / sizeof(*rainbow))

int
color_max(void)
{
  return NCOLORS;
}

char const*
color_get(int i)
{
  return rainbow[i];
}

char const*
color_random(void)
{
  return rainbow[os_rand_range(NCOLORS)];
}
