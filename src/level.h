#ifndef _ROGUE14_LEVEL_H_
#define _ROGUE14_LEVEL_H_

#include "io.h"

#define AMULETLEVEL  26    /* Level where we can find the amulet */

/* TODO: Make functions of these */
#define INDEX(y,x)	(&places[((x) << 5) + (y)])

/* Flags for level map */
#define F_PASS		0x80		/* is a passageway */
#define F_SEEN		0x40		/* have seen this spot before */
#define F_DROPPED	0x20		/* object was dropped here */
#define F_LOCKED	0x20		/* door is locked */
#define F_REAL		0x10		/* what you see is what you get */
#define F_PNUM		0x0f		/* passage number mask */
#define F_TMASK		0x07		/* trap number mask */

/* describe a place on the level map */
typedef struct {
    char p_ch;
    char p_flags;
    THING* p_monst;
} PLACE;

extern PLACE places[MAXLINES*MAXCOLS];  /* level map */
extern coord stairs;   /* Location of staircase */
extern THING* lvl_obj; /* List of objects on this level */

extern int level;      /* What level she is on */
extern int max_level;  /* Deepest player has gone */

void level_new(void);
bool level_save_state(void);
bool level_load_state(void);

char level_get_type(int y, int x);

THING* level_get_monster(int y, int x);
void level_set_monster(int y, int x, THING* monster);

char level_get_flags(int y, int x);
void level_set_flags(int y, int x, char flags);

char level_get_ch(int y, int x);
void level_set_ch(int y, int x, char ch);

#endif /* _ROGUE14_LEVEL_H_ */
