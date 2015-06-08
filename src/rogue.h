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

/* All the fun defines */
#define max(a,b)	((a) > (b) ? (a) : (b))

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
extern bool door_stop;
extern bool firstmove;
extern bool running;
extern bool to_death;

extern char dir_ch;
extern char file_name[];
extern char whoami[];
extern char runch;

extern int wizard;

extern unsigned seed;

extern coord oldpos;

#endif /* _ROGUE14_ROGUE_H_ */
