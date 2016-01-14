#include <stdbool.h>

#include "coord.h"

bool
coord_same(coord const* a, coord const* b)
{
  return a->x == b->x && a->y == b->y;
}
