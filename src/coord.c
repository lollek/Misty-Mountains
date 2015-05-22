#include <stdbool.h>

#include "coord.h"

bool
same_coords(coord const* a, coord const* b)
{
  return a->x == b->x && a->y == b->y;
}
