#pragma once

#include "rogue.h"

extern Coordinate move_pos_prev; /* Position before last look() call */

/* Check to see that a move is legal.  If it is handle the 
 * consequences (fighting, picking up, etc.) */
bool move_do(char ch);
