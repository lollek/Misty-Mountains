#ifndef _ROGUE14_CHASE_H_
#define _ROGUE14_CHASE_H_

#include "rogue.h"

void runners(void); /* Make all running monsters move */
int move_monst(THING *tp); /* Execute a signel turn of running */
void relocate(THING *th, coord *new_loc); /* Relocate monster */
int do_chase(THING *th);   /* Make one thing chase another */
void set_oldch(THING *tp, coord *cp); /* Set oldch for a monster */
bool see_monst(THING *mp);  /* Can player see the monster? */
void runto(coord *runner);  /* Make monster chase hero */

/** Chase
 * Find the spot for the chaser(er) to move closer to the chasee(ee). 
 * Returns true if we want to keep on chasing later */
bool chase(THING *tp, coord *ee);

/** Roomin
 * Find what room some coords are in, NULL means no room */
struct room *roomin(coord *cp);

bool diag_ok(coord *sp, coord *ep); /* Check if move is legal if diagonal */
bool cansee(int y, int x); /* True if player can see coord */
coord *find_dest(THING *tp); /* Find proper destination for monster */

/** Dist
 * Calculate the "distance" between to points.  Actually,
 * this calculates d^2, not d, but that's good enough for
 * our purposes, since it's only used comparitively. */
int dist(int y1, int x1, int y2, int x2);
inline int dist_cp(coord *c1, coord *c2); /* Same as above, but with coords */



#endif /* _ROGUE14_CHASE_H_ */
