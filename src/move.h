#ifndef _ROGUE14_MOVE_H_
#define _ROGUE14_MOVE_H_

#include "rogue.h"

extern coord move_pos_prev; /* Position before last look() call */

void move_random(THING* who, coord* ret);

/* Check to see that a move is legal.  If it is handle the 
 * consequences (fighting, picking up, etc.) */
bool move_do(char ch);

#endif /* _ROGUE14_MOVE_H_ */
