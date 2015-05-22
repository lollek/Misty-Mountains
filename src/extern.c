/*
 * global variable initializaton
 *
 * @(#)extern.c	4.82 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include "io.h"
#include "coord.h"

bool after = false;			/* True if we want after daemons */
bool again = false;			/* Repeating the last command */
bool door_stop = false;			/* Stop running when we pass a door */
bool firstmove = false;			/* First move after setting door_stop */
bool has_hit = false;			/* Has a "hit" message pending in msg */
bool kamikaze = false;			/* to_death really to DEATH */
bool running = false;			/* True if player is running */
bool to_death = false;			/* Fighting is to the death! */
int wizard = false;			/* True if allows wizard commands */

char dir_ch;				/* Direction from last get_dir() call */
char file_name[MAXSTR];			/* Save file name */
char runch;				/* Direction player is running */
char whoami[MAXSTR];			/* Name of player */
char l_last_comm = '\0';		/* Last last_comm */
char l_last_dir = '\0';			/* Last last_dir */
char last_comm = '\0';			/* Last command typed */
char last_dir = '\0';			/* Last direction given */

unsigned seed;				/* Random number seed */

coord oldpos;				/* Position before last look() call */

