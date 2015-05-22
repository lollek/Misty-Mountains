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

#include "rogue.h"

enum rogue_game_t game_type;            /* Which variant to play */

bool after = false;			/* True if we want after daemons */
bool again = false;			/* Repeating the last command */
bool door_stop = false;			/* Stop running when we pass a door */
bool firstmove = false;			/* First move after setting door_stop */
bool has_hit = false;			/* Has a "hit" message pending in msg */
bool kamikaze = false;			/* to_death really to DEATH */
bool move_on = false;			/* Next move shouldn't pick up items */
bool running = false;			/* True if player is running */
bool to_death = false;			/* Fighting is to the death! */
int wizard = false;			/* True if allows wizard commands */

char dir_ch;				/* Direction from last get_dir() call */
char file_name[MAXSTR];			/* Save file name */
char huh[MAXSTR] = { '\0' };		/* The last message printed */
char runch;				/* Direction player is running */
char take;				/* Thing she is taking */
char whoami[MAXSTR];			/* Name of player */
char l_last_comm = '\0';		/* Last last_comm */
char l_last_dir = '\0';			/* Last last_dir */
char last_comm = '\0';			/* Last command typed */
char last_dir = '\0';			/* Last direction given */

int hungry_state = 0;			/* How hungry is he */
int mpos = 0;				/* Where cursor is on top line */
int no_food = 0;			/* Number of levels without food */

int food_left;				/* Amount of food in hero's stomach */
int no_command = 0;			/* Number of turns asleep */
int no_move = 0;			/* Number of turns held in place */
int purse = 0;				/* How much gold he has */
int vf_hit = 0;				/* Number of time flytrap has hit */
unsigned seed;				/* Random number seed */

coord oldpos;				/* Position before last look() call */

