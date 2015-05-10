#ifndef _ROGUE14_COORD_H_
#define _ROGUE14_COORD_H_

/* Coordinate data type */
typedef struct {
    int x;
    int y;
} coord;

#define same_coords(a,b) ((a).x == (b).x && (a).y == (b).y)

#endif /* _ROGUE14_COORD_H_ */
