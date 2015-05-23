#ifndef _ROGUE14_ROGUE_H_
#define _ROGUE14_ROGUE_H_
/*
 * Rogue definitions and variable declarations
 *
 * @(#)rogue.h	5.42 (Berkeley) 08/06/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdbool.h>
#include <ncurses.h>

#include "coord.h"
#include "things.h"

/* Tuneable - feel free to change these */
#define NUMNAME    "Ten"   /* The same number in letters  */
#define NUMSCORES    10    /* Number of highscore entries */
#define AMULETLEVEL  26    /* Level where we can find the amulet */

/* Try not to change these */
#define PACKSIZE 22 /* How many items we can have in our pack */

/* All the fun defines */
#define winat(y,x)	(moat(y,x) != NULL ? moat(y,x)->t_disguise : chat(y,x))
#define max(a,b)	((a) > (b) ? (a) : (b))
#define GOLDCALC	(rnd(50 + 10 * level) + 2)

/* Various constants */
#define WANDERTIME	spread(70)
#define BEFORE		spread(1)
#define AFTER		spread(2)
#define HUNGERTIME	1300
#define STOMACHSIZE	2000
#define STARVETIME	850
#define LEFT		0
#define RIGHT		1
#define BOLT_LENGTH	6
#define LAMPDIST	3

/* Save against things */
#define VS_POISON	00
#define VS_MAGIC	03


/* External variables */

extern bool after;
extern bool again;
extern bool door_stop;
extern bool firstmove;
extern bool has_hit;
extern bool kamikaze;
extern bool running;
extern bool to_death;

extern char dir_ch;
extern char file_name[];
extern char whoami[];
extern char l_last_comm;
extern char l_last_dir;
extern char last_comm;
extern char last_dir;
extern char runch;

extern int wizard;

extern unsigned seed;

extern coord oldpos;

#endif /* _ROGUE14_ROGUE_H_ */
