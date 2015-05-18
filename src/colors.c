#include <stdlib.h>
#include <stdbool.h>

#include "potions.h"
#include "misc.h"
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

void
colors_init(void)
{
  bool used[NCOLORS];

  for (size_t i = 0; i < NCOLORS; i++)
    used[i] = false;

  for (size_t i = 0; i < NPOTIONS; i++)
  {
    size_t j;
    do
      j = rnd(NCOLORS);
    while (used[j]);
    used[j] = true;
    p_colors[i] = rainbow[j];
  }
}

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
  return rainbow[rnd(NCOLORS)];
}
