#ifndef ROGUE14_MOVE_H
#define ROGUE14_MOVE_H

#include "rogue.h"

extern Coordinate move_pos_prev; /* Position before last look() call */

void move_random(monster* who, Coordinate* ret);

/* Check to see that a move is legal.  If it is handle the 
 * consequences (fighting, picking up, etc.) */
bool move_do(char ch);

#endif /* ROGUE14_MOVE_H */
