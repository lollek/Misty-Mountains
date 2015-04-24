#ifndef _ROGUE14_MOVE_H_
#define _ROGUE14_MOVE_H_

#include "rogue.h"

void move_random(THING *who, coord *ret);

/* Start the hero running */
bool move_do_run(char ch, bool cautiously);

/* Check to see that a move is legal.  If it is handle the 
 * consequences (fighting, picking up, etc.) */
bool move_do(char ch);

#endif /* _ROGUE14_MOVE_H_ */
