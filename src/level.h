#ifndef _ROGUE14_LEVEL_H_
#define _ROGUE14_LEVEL_H_

#include "rogue.h"

/* TODO: Make functions of these */
#define INDEX(y,x)	(&places[((x) << 5) + (y)])
#define chat(y,x)	(places[((x) << 5) + (y)].p_ch)
#define flat(y,x)	(places[((x) << 5) + (y)].p_flags)
#define moat(y,x)	(places[((x) << 5) + (y)].p_monst)

PLACE places[MAXLINES*MAXCOLS];  /* level map */
coord stairs;   /* Location of staircase */
THING *lvl_obj; /* List of objects on this level */
int level;      /* What level she is on */
int max_level;  /* Deepest player has gone */

void level_new(void);
bool level_save_state(void);
bool level_load_state(void);

#endif /* _ROGUE14_LEVEL_H_ */
