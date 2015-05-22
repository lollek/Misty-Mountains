#ifndef _ROGUE14_LEVEL_H_
#define _ROGUE14_LEVEL_H_

#include "rogue.h"

/* TODO: Make functions of these */
#define INDEX(y,x)	(&places[((x) << 5) + (y)])
#define chat(y,x)	(places[((x) << 5) + (y)].p_ch)
#define flat(y,x)	(places[((x) << 5) + (y)].p_flags)
#define moat(y,x)	(places[((x) << 5) + (y)].p_monst)

/* Flags for level map */
#define F_PASS		0x80		/* is a passageway */
#define F_SEEN		0x40		/* have seen this spot before */
#define F_DROPPED	0x20		/* object was dropped here */
#define F_LOCKED	0x20		/* door is locked */
#define F_REAL		0x10		/* what you see is what you get */
#define F_PNUM		0x0f		/* passage number mask */
#define F_TMASK		0x07		/* trap number mask */

PLACE places[MAXLINES*MAXCOLS];  /* level map */
coord stairs;   /* Location of staircase */
THING* lvl_obj; /* List of objects on this level */

int level;      /* What level she is on */
int max_level;  /* Deepest player has gone */

void level_new(void);
bool level_save_state(void);
bool level_load_state(void);

#endif /* _ROGUE14_LEVEL_H_ */
