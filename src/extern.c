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

bool after;				/* True if we want after daemons */
bool again;				/* Repeating the last command */
int  noscore;				/* Was a wizard sometime */
bool seenstairs;			/* Have seen the stairs (for lsd) */
bool amulet = false;			/* He found the amulet */
bool door_stop = false;			/* Stop running when we pass a door */
bool firstmove = false;			/* First move after setting door_stop */
bool has_hit = false;			/* Has a "hit" message pending in msg */
bool inv_describe = TRUE;		/* Say which way items are being used */
bool kamikaze = false;			/* to_death really to DEATH */
bool lower_msg = false;			/* Messages should start w/lower case */
bool move_on = false;			/* Next move shouldn't pick up items */
bool msg_esc = false;			/* Check for ESC from msg's --More-- */
bool playing = TRUE;			/* True until he quits */
bool q_comm = false;			/* Are we executing a 'Q' command? */
bool running = false;			/* True if player is running */
bool save_msg = TRUE;			/* Remember last msg */
bool stat_msg = false;			/* Should status() print as a msg() */
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
char huh[MAXSTR];			/* The last message printed */
char prbuf[2*MAXSTR];			/* buffer for sprintfs */
char *r_stones[MAXRINGS];		/* Stone settings of the rings */
char runch;				/* Direction player is running */
char take;				/* Thing she is taking */
char whoami[MAXSTR];			/* Name of player */
char *ws_made[MAXSTICKS];		/* What sticks are made of */
char *ws_type[MAXSTICKS];		/* Is it a wand or a staff */
char *inv_t_name[] = {
	"Overwrite",
	"Slow",
	"Clear"
};
char l_last_comm = '\0';		/* Last last_comm */
char l_last_dir = '\0';			/* Last last_dir */
char last_comm = '\0';			/* Last command typed */
char last_dir = '\0';			/* Last direction given */
char *tr_name[] = {			/* Names of the traps */
	"a trapdoor",
	"an arrow trap",
	"a sleeping gas trap",
	"a beartrap",
	"a teleport trap",
	"a poison dart trap",
	"a rust trap",
        "a mysterious trap"
};


int n_objs;				/* # items listed in inventory() call */
int ntraps;				/* Number of traps on this level */
int hungry_state = 0;			/* How hungry is he */
int inpack = 0;				/* Number of things in pack */
int level = 1;				/* What level she is on */
int max_hit;				/* Max damage done to her in to_death */
int max_level;				/* Deepest player has gone */
int mpos = 0;				/* Where cursor is on top line */
int no_food = 0;			/* Number of levels without food */
int a_class[MAXARMORS] = {		/* Armor class for each armor type */
	8,	/* LEATHER */
	7,	/* RING_MAIL */
	7,	/* STUDDED_LEATHER */
	6,	/* SCALE_MAIL */
	5,	/* CHAIN_MAIL */
	4,	/* SPLINT_MAIL */
	4,	/* BANDED_MAIL */
	3,	/* PLATE_MAIL */
};

int count = 0;				/* Number of times to repeat command */
int food_left;				/* Amount of food in hero's stomach */
int lastscore = -1;			/* Score before this turn */
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

THING *cur_armor;			/* What he is wearing */
THING *cur_ring[2];			/* Which rings are being worn */
THING *cur_weapon;			/* Which weapon he is weilding */
THING *l_last_pick = NULL;		/* Last last_pick */
THING *last_pick = NULL;		/* Last object picked in get_item() */
THING *lvl_obj = NULL;			/* List of objects on this level */
THING *mlist = NULL;			/* List of monsters on the level */
THING player;				/* His stats */
					/* restart of game */

WINDOW *hw = NULL;			/* used as a scratch window */

#define INIT_STATS { 16, 0, 1, 10, 12, "1x4", 12 }

struct stats max_stats = INIT_STATS;	/* The maximum for the player */

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

#define ___ 1
#define XX 10
struct monster monsters[26] =
    {
/* Name		 CARRY	FLAG    str, exp, lvl, amr, hpt, dmg,      ,maxhp */
{ "aquator",	   0,	ISMEAN,	{ XX, 20,   5,   2, ___, "0x0/0x0", 0 } },
{ "bat",	   0,	ISFLY,	{ XX,  1,   1,   3, ___, "1x2", 0 } },
{ "centaur",	  15,	0,	{ XX, 17,   4,   4, ___, "1x2/1x5/1x5", 0 } },
{ "dragon",	 100,	ISMEAN,	{ XX,5000, 10,  -1, ___, "1x8/1x8/3x10", 0 } },
{ "emu",	   0,	ISMEAN,	{ XX,  2,   1,   7, ___, "1x2", 0 } },
{ "venus flytrap", 0,	ISMEAN,	{ XX, 80,   8,   3, ___, "%%%x0", 0 } },
	/* NOTE: the damage is %%% so that xstr won't merge this */
	/* string with others, since it is written on in the program */
{ "griffin",	  20,	ISMEAN|ISFLY|ISREGEN, { XX,2000, 13,   2, ___, "4x3/3x5", 0 } },
{ "hobgoblin",	   0,	ISMEAN,	{ XX,  3,   1,   5, ___, "1x8", 0 } },
{ "ice monster",   0,	0,	{ XX,  5,   1,   9, ___, "0x0", 0 } },
{ "jabberwock",   70,	0,	{ XX,3000, 15,   6, ___, "2x12/2x4", 0 } },
{ "kestrel",	   0,	ISMEAN|ISFLY,	{ XX,  1,   1,   7, ___, "1x4", 0 } },
{ "leprechaun",	   0,	0,	{ XX, 10,   3,   8, ___, "1x1", 0 } },
{ "medusa",	  40,	ISMEAN,	{ XX,200,   8,   2, ___, "3x4/3x4/2x5", 0 } },
{ "nymph",	 100,	0,	{ XX, 37,   3,   9, ___, "0x0", 0 } },
{ "orc",	  15,	ISGREED,{ XX,  5,   1,   6, ___, "1x8", 0 } },
{ "phantom",	   0,	ISINVIS,{ XX,120,   8,   3, ___, "4x4", 0 } },
{ "quagga",	   0,	ISMEAN,	{ XX, 15,   3,   3, ___, "1x5/1x5", 0 } },
{ "rattlesnake",   0,	ISMEAN,	{ XX,  9,   2,   3, ___, "1x6", 0 } },
{ "snake",	   0,	ISMEAN,	{ XX,  2,   1,   5, ___, "1x3", 0 } },
{ "troll",	  50,	ISREGEN|ISMEAN,{ XX, 120, 6, 4, ___, "1x8/1x8/2x6", 0 } },
{ "black unicorn", 0,	ISMEAN,	{ XX,190,   7,  -2, ___, "1x9/1x9/2x9", 0 } },
{ "vampire",	  20,	ISREGEN|ISMEAN,{ XX,350,   8,   1, ___, "1x10", 0 } },
{ "wraith",	   0,	0,	{ XX, 55,   5,   4, ___, "1x6", 0 } },
{ "xeroc",	  30,	0,	{ XX,100,   7,   7, ___, "4x4", 0 } },
{ "yeti",	  30,	0,	{ XX, 50,   4,   6, ___, "1x6/1x6", 0 } },
{ "zombie",	   0,	ISMEAN,	{ XX,  6,   2,   8, ___, "1x8", 0 } }
    };
#undef ___
#undef XX

    /* Dev info: Only the first two vales are used in this table,
     *           the others need not be initialized */
struct obj_info things[NUMTHINGS] = {
    { 0,	26,	0,	NULL,	false },	/* potion */
    { 0,	36,	0,	NULL,	false },	/* scroll */
    { 0,	16,	0,	NULL,	false },	/* food */
    { 0,	 7,	0,	NULL,	false },	/* weapon */
    { 0,	 7,	0,	NULL,	false },	/* armor */
    { 0,	 4,	0,	NULL,	false },	/* ring */
    { 0,	 4,	0,	NULL,	false },	/* stick */
};

struct obj_info arm_info[MAXARMORS] = {
    { "leather armor",		 20,	 20, NULL, false },
    { "ring mail",		 15,	 25, NULL, false },
    { "studded leather armor",	 15,	 20, NULL, false },
    { "scale mail",		 13,	 30, NULL, false },
    { "chain mail",		 12,	 75, NULL, false },
    { "splint mail",		 10,	 80, NULL, false },
    { "banded mail",		 10,	 90, NULL, false },
    { "plate mail",		  5,	150, NULL, false },
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

struct h_list helpstr[] = {
    {'?',	"	prints help",				TRUE},
    {'/',	"	identify object",			TRUE},
    {'h',	"	left",					TRUE},
    {'j',	"	down",					TRUE},
    {'k',	"	up",					TRUE},
    {'l',	"	right",					TRUE},
    {'y',	"	up & left",				TRUE},
    {'u',	"	up & right",				TRUE},
    {'b',	"	down & left",				TRUE},
    {'n',	"	down & right",				TRUE},
    {'H',	"	run left",				false},
    {'J',	"	run down",				false},
    {'K',	"	run up",				false},
    {'L',	"	run right",				false},
    {'Y',	"	run up & left",				false},
    {'U',	"	run up & right",			false},
    {'B',	"	run down & left",			false},
    {'N',	"	run down & right",			false},
    {CTRL('H'),	"	run left until adjacent",		false},
    {CTRL('J'),	"	run down until adjacent",		false},
    {CTRL('K'),	"	run up until adjacent",			false},
    {CTRL('L'),	"	run right until adjacent",		false},
    {CTRL('Y'),	"	run up & left until adjacent",		false},
    {CTRL('U'),	"	run up & right until adjacent",		false},
    {CTRL('B'),	"	run down & left until adjacent",	false},
    {CTRL('N'),	"	run down & right until adjacent",	false},
    {'\0',	"	<SHIFT><dir>: run that way",		TRUE},
    {'\0',	"	<CTRL><dir>: run till adjacent",	TRUE},
    {'f',	"<dir>	fight till death or near death",	TRUE},
    {'t',	"<dir>	throw something",			TRUE},
    {'m',	"<dir>	move onto without picking up",		TRUE},
    {'z',	"<dir>	zap a wand in a direction",		TRUE},
    {'^',	"<dir>	identify trap type",			TRUE},
    {'s',	"	search for trap/secret door",		TRUE},
    {'>',	"	go down a staircase",			TRUE},
    {'<',	"	go up a staircase",			TRUE},
    {'.',	"	rest for a turn",			TRUE},
    {',',	"	pick something up",			TRUE},
    {'i',	"	inventory",				TRUE},
    {'I',	"	inventory single item",			TRUE},
    {'q',	"	quaff potion",				TRUE},
    {'r',	"	read scroll",				TRUE},
    {'e',	"	eat food",				TRUE},
    {'w',	"	wield a weapon",			TRUE},
    {'W',	"	wear armor",				TRUE},
    {'T',	"	take armor off",			TRUE},
    {'P',	"	put on ring",				TRUE},
    {'R',	"	remove ring",				TRUE},
    {'d',	"	drop object",				TRUE},
    {'c',	"	call object",				TRUE},
    {'a',	"	repeat last command",			TRUE},
    {')',	"	print current weapon",			TRUE},
    {']',	"	print current armor",			TRUE},
    {'=',	"	print current rings",			TRUE},
    {'@',	"	print current stats",			TRUE},
    {'D',	"	recall what's been discovered",		TRUE},
    {'o',	"	examine/set options",			TRUE},
    {CTRL('R'),	"	redraw screen",				TRUE},
    {CTRL('P'),	"	repeat last message",			TRUE},
    {ESCAPE,	"	cancel command",			TRUE},
    {'S',	"	save game",				TRUE},
    {'Q',	"	quit",					TRUE},
    {'!',	"	shell escape",				TRUE},
    {'F',	"<dir>	fight till either of you dies",		TRUE},
    { 0 ,		NULL,					false}
};
