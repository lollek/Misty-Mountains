#pragma once

#include <list>

#include "io.h"

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
    char     p_ch;
    char     p_flags;
    monster* p_monst;
} PLACE;

extern int const level_amulet;                    /* Level where amulet starts */

extern PLACE              level_places[MAXLINES*MAXCOLS];  /* level map */
extern Coordinate         level_stairs;                    /* Location of staircase */
extern std::list<item*>   level_items;                     /* List of items on level */
extern int                level;                           /* What level she is on */
extern int                level_max;                       /* Deepest player has gone */
extern int                levels_without_food;             /* Levels without food */

void level_new(void);
bool level_save_state(void);
bool level_load_state(void);

char level_get_type(int y, int x);

PLACE* level_get_place(int y, int x);

monster* level_get_monster(int y, int x);
void level_set_monster(int y, int x, monster* monster);

char level_get_flags(int y, int x);
void level_set_flags(int y, int x, char flags);

char level_get_ch(int y, int x);
void level_set_ch(int y, int x, char ch);
