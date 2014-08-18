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
bool msg_esc = false;			/* Check for ESC from msg's --More-- */
bool running = false;			/* True if player is running */
bool to_death = false;			/* Fighting is to the death! */
int wizard = false;			/* True if allows wizard commands */
int potential_wizard = false;		/* True if allows to become a wizard */
bool pack_used[26] = {			/* Is the character used in the pack? */
    false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false
};

char dir_ch;				/* Direction from last get_dir() call */
char file_name[MAXSTR];			/* Save file name */
char huh[MAXSTR] = { '\0' };		/* The last message printed */
char prbuf[2*MAXSTR];			/* buffer for sprintfs */
char *r_stones[MAXRINGS];		/* Stone settings of the rings */
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
int quiet = 0;				/* Number of quiet turns */
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

PLACE places[MAXLINES*MAXCOLS];		/* level map */

THING *l_last_pick = NULL;		/* Last last_pick */
THING *last_pick = NULL;		/* Last object picked in get_item() */
THING *lvl_obj = NULL;			/* List of objects on this level */
THING *mlist = NULL;			/* List of monsters on the level */
THING player;				/* His stats */
					/* restart of game */

WINDOW *hw = NULL;			/* used as a scratch window */

/* The maximum for the player */
struct stats max_stats = { 16, 0, 1, 10, 12, "1x4", 12 };

struct room *oldrp;			/* Roomin(&oldpos) */
struct room rooms[MAXROOMS];		/* One for each room -- A level */
struct room passages[MAXPASS] =		/* One for each passage */
{
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, {{0,0}} }
};

struct monster monsters[26] =
    {
/* Name		 CARRY	FLAG    str, exp, lvl, amr, hpt, dmg,      ,maxhp */
{ "aquator",	   0,	ISMEAN,	{ 10, 20,   5,   2,  1, "0x0/0x0", 0 } },
{ "bat",	   0,	ISFLY,	{ 10,  1,   1,   3,  1, "1x2", 0 } },
{ "centaur",	  15,	0,	{ 10, 17,   4,   4,  1, "1x2/1x5/1x5", 0 } },
{ "dragon",	 100,	ISMEAN,	{ 10,5000, 10,  -1,  1, "1x8/1x8/3x10", 0 } },
{ "emu",	   0,	ISMEAN,	{ 10,  2,   1,   7,  1, "1x2", 0 } },
{ "venus flytrap", 0,	ISMEAN,	{ 10, 80,   8,   3,  1, "000x0", 0 } },
{ "griffin",	  20,	ISMEAN|ISFLY|ISREGEN, { 10,2000, 13,   2, 1, "4x3/3x5", 0 } },
{ "hobgoblin",	   0,	ISMEAN,	{ 10,  3,   1,   5,  1, "1x8", 0 } },
{ "ice monster",   0,	0,	{ 10,  5,   1,   9,  1, "0x0", 0 } },
{ "jabberwock",   70,	0,	{ 10,3000, 15,   6,  1, "2x12/2x4", 0 } },
{ "kestrel",	   0,	ISMEAN|ISFLY,	{ 10,  1,    1,   7, 1, "1x4", 0 } },
{ "leprechaun",	   0,	0,	{ 10, 10,   3,   8,  1, "1x1", 0 } },
{ "medusa",	  40,	ISMEAN,	{ 10,200,   8,   2,  1, "3x4/3x4/2x5", 0 } },
{ "nymph",	 100,	0,	{ 10, 37,   3,   9,  1, "0x0", 0 } },
{ "orc",	  15,	ISGREED,{ 10,  5,   1,   6,  1, "1x8", 0 } },
{ "phantom",	   0,	ISINVIS,{ 10,120,   8,   3,  1, "4x4", 0 } },
{ "quagga",	   0,	ISMEAN,	{ 10, 15,   3,   3,  1, "1x5/1x5", 0 } },
{ "rattlesnake",   0,	ISMEAN,	{ 10,  9,   2,   3,  1, "1x6", 0 } },
{ "snake",	   0,	ISMEAN,	{ 10,  2,   1,   5,  1, "1x3", 0 } },
{ "troll",	  50,	ISREGEN|ISMEAN,{ 10, 120, 6, 4, 1, "1x8/1x8/2x6", 0 } },
{ "black unicorn", 0,	ISMEAN,	{ 10,190,   7,  -2, 1, "1x9/1x9/2x9", 0 } },
{ "vampire",	  20,	ISREGEN|ISMEAN,{ 10,350,   8,   1, 1, "1x10", 0 } },
{ "wraith",	   0,	0,	{ 10, 55,   5,   4,  1, "1x6", 0 } },
{ "xeroc",	  30,	0,	{ 10,100,   7,   7,  1, "4x4", 0 } },
{ "yeti",	  30,	0,	{ 10, 50,   4,   6,  1, "1x6/1x6", 0 } },
{ "zombie",	   0,	ISMEAN,	{ 10,  6,   2,   8,  1, "1x8", 0 } }
    };

/* Only oi_prob is used
 *    oi_name  oi_prob oi_worth oi_guess oi_know */
struct obj_info things[] = {
    { "potion",	26,	0,	NULL,	false },	/* potion */
    { "scroll",	36,	0,	NULL,	false },	/* scroll */
    { "food",	16,	0,	NULL,	false },	/* food */
    { "weapon",	 7,	0,	NULL,	false },	/* weapon */
    { "armor",	 7,	0,	NULL,	false },	/* armor */
    { "ring",	 4,	0,	NULL,	false },	/* ring */
    { "stick",	 4,	0,	NULL,	false },	/* stick */
};

struct obj_info ring_info[MAXRINGS] = {
    { "protection",		 9, 400, NULL, false },
    { "add strength",		 9, 400, NULL, false },
    { "sustain strength",	 5, 280, NULL, false },
    { "searching",		10, 420, NULL, false },
    { "see invisible",		10, 310, NULL, false },
    { "adornment",		 1,  10, NULL, false },
    { "aggravate monster",	10,  10, NULL, false },
    { "dexterity",		 8, 440, NULL, false },
    { "increase damage",	 8, 400, NULL, false },
    { "regeneration",		 4, 460, NULL, false },
    { "slow digestion",		 9, 240, NULL, false },
    { "teleportation",		 5,  30, NULL, false },
    { "stealth",		 7, 470, NULL, false },
    { "maintain armor",		 5, 380, NULL, false },
};

struct obj_info weap_info[MAXWEAPONS + 1] = {
    { "mace",				11,   8, NULL, false },
    { "long sword",			11,  15, NULL, false },
    { "short bow",			12,  15, NULL, false },
    { "arrow",				12,   1, NULL, false },
    { "dagger",				 8,   3, NULL, false },
    { "two handed sword",		10,  75, NULL, false },
    { "dart",				12,   2, NULL, false },
    { "shuriken",			12,   5, NULL, false },
    { "spear",				12,   5, NULL, false },
    /* DO NOT REMOVE: fake entry for dragon's breath */
    { NULL,				0,    0, NULL, false },	
};

struct obj_info ws_info[MAXSTICKS] = {
    { "light",			12, 250, NULL, false },
    { "invisibility",		 6,   5, NULL, false },
    { "lightning",		 3, 330, NULL, false },
    { "fire",			 3, 330, NULL, false },
    { "cold",			 3, 330, NULL, false },
    { "polymorph",		15, 310, NULL, false },
    { "magic missile",		10, 170, NULL, false },
    { "haste monster",		10,   5, NULL, false },
    { "slow monster",		11, 350, NULL, false },
    { "drain life",		 9, 300, NULL, false },
    { "nothing",		 1,   5, NULL, false },
    { "teleport away",		 6, 340, NULL, false },
    { "teleport to",		 6,  50, NULL, false },
    { "cancellation",		 5, 280, NULL, false },
};
