#ifndef ROGUE14_COORD_H
#define ROGUE14_COORD_H

#include <stdbool.h>

/* Coordinate data type */
typedef struct {
    int x;
    int y;
} coord;

bool coord_same(coord const* a, coord const* b);

#endif /* ROGUE14_COORD_H */
