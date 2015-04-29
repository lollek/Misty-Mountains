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

const char *game_version = "Rogue14 r" VERSION " - Based on Rogue5.4.4";

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
int potential_wizard = false;		/* True if allows to become a wizard */

char dir_ch;				/* Direction from last get_dir() call */
char file_name[MAXSTR];			/* Save file name */
char huh[MAXSTR] = { '\0' };		/* The last message printed */
char prbuf[2*MAXSTR];			/* buffer for sprintfs */
char runch;				/* Direction player is running */
char take;				/* Thing she is taking */
char whoami[MAXSTR];			/* Name of player */
char *ws_made[MAXSTICKS];		/* What sticks are made of */
char *ws_type[MAXSTICKS];		/* Is it a wand or a staff */
char l_last_comm = '\0';		/* Last last_comm */
char l_last_dir = '\0';			/* Last last_dir */
char last_comm = '\0';			/* Last command typed */
char last_dir = '\0';			/* Last direction given */

int hungry_state = 0;			/* How hungry is he */
int level = 1;				/* What level she is on */
int max_hit;				/* Max damage done to her in to_death */
int max_level;				/* Deepest player has gone */
int mpos = 0;				/* Where cursor is on top line */
int no_food = 0;			/* Number of levels without food */

int count = 0;				/* Number of times to repeat command */
int food_left;				/* Amount of food in hero's stomach */
int no_command = 0;			/* Number of turns asleep */
int no_move = 0;			/* Number of turns held in place */
int purse = 0;				/* How much gold he has */
int vf_hit = 0;				/* Number of time flytrap has hit */
unsigned seed;				/* Random number seed */
int e_levels[] = {
        10L,
	20L,
	40L,
	80L,
       160L,
       320L,
       640L,
      1300L,
      2600L,
      5200L,
     13000L,
     26000L,
     50000L,
    100000L,
    200000L,
    400000L,
    800000L,
   2000000L,
   4000000L,
   8000000L,
	 0L
};

coord delta;				/* Change indicated to get_dir() */
coord oldpos;				/* Position before last look() call */
coord stairs;				/* Location of staircase */

THING *l_last_pick = NULL;		/* Last last_pick */
THING *last_pick = NULL;		/* Last object picked in pack_get_item() */
THING *lvl_obj = NULL;			/* List of objects on this level */
THING *mlist = NULL;			/* List of monsters on the level */
THING player;				/* His stats */
					/* restart of game */

WINDOW *hw = NULL;			/* used as a scratch window */

/* The maximum for the player */
struct stats max_stats = { 16, 0, 1, 10, 12, "1x4", 12 };

