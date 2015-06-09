#ifndef _ROGUE14_COORD_H_
#define _ROGUE14_COORD_H_

#include <stdbool.h>

/* Coordinate data type */
typedef struct {
    int x;
    int y;
} coord;

bool coord_same(coord const* a, coord const* b);

#endif /* _ROGUE14_COORD_H_ */
