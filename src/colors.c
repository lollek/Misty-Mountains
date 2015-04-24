#include <stdlib.h>
#include <stdbool.h>

#include "potions.h"
#include "rogue.h"

#include "colors.h"

static char *rainbow[] = {
    "amber",     "aquamarine", "black",      "blue",       "brown",
    "clear",     "crimson",    "cyan",       "ecru",       "gold",
    "green",     "grey",       "magenta",    "orange",     "pink",
    "plaid",     "purple",     "red",        "silver",     "tan",
    "tangerine", "topaz",      "turquoise",  "vermilion",  "violet",
    "white",     "yellow",
};

#define NCOLORS (sizeof(rainbow) / sizeof(*rainbow))

void *__colors_ptr(void) { return rainbow; }
size_t __colors_size(void) { return NCOLORS; }


void
colors_init(void)
{
  size_t i;
  bool used[NCOLORS];

  for (i = 0; i < NCOLORS; i++)
    used[i] = false;

  for (i = 0; i < NPOTIONS; i++)
  {
    size_t j;
    do
      j = rnd(NCOLORS);
    while (used[j]);
    used[j] = true;
    p_colors[i] = rainbow[j];
  }
}

char *colors_random(void)
{
  return rainbow[rnd(NCOLORS)];
}
