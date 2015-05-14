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

/* Version */
#define GAME_VERSION "Rogue14 " VERSION " - Based on Rogue5.4.4"

/* Tuneable - feel free to change these */
#define NUMNAME    "Ten"   /* The same number in letters  */
#define NUMSCORES    10    /* Number of highscore entries */
#define AMULETLEVEL  26    /* Level where we can find the amulet */

/* Try not to change these */
#define MAXSTR 1024 /* maximum length of strings */
#define MAXCOLS  80 /* maximum number of screen columns used */
#define MAXINP   50 /* max string to read from terminal or environment */
#define MAXLINES 32 /* maximum number of screen lines used */
#define NUMLINES 24
#define NUMCOLS  80
#define PACKSIZE 22 /* How many items we can have in our pack */
#define STATLINE (NUMLINES - 1)

extern enum rogue_game_t
{
  DEFAULT,
  QUICK
} game_type;

/* All the fun defines */
#define winat(y,x)	(moat(y,x) != NULL ? moat(y,x)->t_disguise : chat(y,x))
#define max(a,b)	((a) > (b) ? (a) : (b))
#define on(thing,flag)	((bool)(((thing).t_flags & (flag)) != 0))
#define GOLDCALC	(rnd(50 + 10 * level) + 2)

/* things that appear on the screens */
#define SHADOW		' '
#define VWALL		'|'
#define HWALL		'-'
#define PASSAGE		'#'
#define DOOR		'+'
#define FLOOR		'.'
#define PLAYER		'@'
#define TRAP		'^'
#define STAIRS		'%'
#define GOLD		'*'
#define POTION		'!'
#define SCROLL		'?'
#define MAGIC		'$'
#define FOOD		':'
#define WEAPON		')'
#define ARMOR		']'
#define AMULET		','
#define RING		'='
#define STICK		'/'

/* TODO: Make sure these don't bug as chars are usually unsigned */
#define RENAMEABLE	-1 /* This means that we can rename it */
#define R_OR_S		-2 /* This means ring, wand of staff */

/* Various constants */
#define WANDERTIME	spread(70)
#define BEFORE		spread(1)
#define AFTER		spread(2)
#define HUNGERTIME	1300
#define MORETIME	150
#define STOMACHSIZE	2000
#define STARVETIME	850
#define LEFT		0
#define RIGHT		1
#define BOLT_LENGTH	6
#define LAMPDIST	3
#define CONCURRENT_RINGS	2	/* How many rings we can wield */

/* Save against things */
#define VS_POISON	00
#define VS_PARALYZATION	00
#define VS_DEATH	00
#define VS_BREATH	02
#define VS_MAGIC	03

/*
 * Various flag bits
 */
/* flags for rooms */
#define ISDARK	0000001		/* room is dark */
#define ISGONE	0000002		/* room is gone (a corridor) */
#define ISMAZE	0000004		/* room is gone (a corridor) */

/* flags for objects */
#define ISCURSED 000001		/* object is cursed */
#define ISKNOW	0000002		/* player knows details about the object */
#define ISMISL	0000004		/* object is a missile type */
#define ISMANY	0000010		/* object comes in groups */
/*	ISFOUND 0000020		...is used for both objects and creatures */
#define	ISPROT	0000040		/* armor is permanently protected */

/* flags for creatures */
#define CANHUH	0000001		/* creature can confuse */
#define CANSEE	0000002		/* creature can see invisible creatures */
#define ISBLIND	0000004		/* creature is blind */
#define ISCANC	0000010		/* creature has special qualities cancelled */
#define ISLEVIT	0000010		/* hero is levitating */
#define ISFOUND	0000020		/* creature has been seen (used for objects) */
#define ISGREED	0000040		/* creature runs to protect gold */
#define ISHASTE	0000100		/* creature has been hastened */
#define ISTARGET 000200		/* creature is the target of an 'f' command */
#define ISHELD	0000400		/* creature has been held */
#define ISHUH	0001000		/* creature is confused */
#define ISINVIS	0002000		/* creature is invisible */
#define ISMEAN	0004000		/* creature can wake when player enters room */
#define ISHALU	0004000		/* hero is on acid trip */
#define ISREGEN	0010000		/* creature can regenerate */
#define ISRUN	0020000		/* creature is running at the player */
#define SEEMONST 040000		/* hero can detect unseen monsters */
#define ISFLY	0040000		/* creature can fly */
#define ISSLOW	0100000		/* creature has been slowed */
#define ISSTUCK	0200000		/* creature cannot move its feet */

/*
 * Flags for level map
 */
#define F_PASS		0x80		/* is a passageway */
#define F_SEEN		0x40		/* have seen this spot before */
#define F_DROPPED	0x20		/* object was dropped here */
#define F_LOCKED	0x20		/* door is locked */
#define F_REAL		0x10		/* what you see is what you get */
#define F_PNUM		0x0f		/* passage number mask */
#define F_TMASK		0x07		/* trap number mask */

/*
 * Help list
 */
struct h_list {
    char h_ch;
    char *h_desc;
    bool h_print;
};

/*
 * describe a place on the level map
 */
typedef struct {
    char p_ch;
    char p_flags;
    THING *p_monst;
} PLACE;

/* Game Options - These are set in main.c */
bool terse;       /* Terse output */
bool fight_flush; /* Flush typeahead during battle */
bool jump;        /* Show running as a series of jumps */
bool see_floor;   /* Show the lamp-illuminated floor */
bool passgo;      /* Follow the turnings in passageways */
bool tombstone;   /* Print out tombstone when killed */
bool use_colors;  /* Use ncurses colors */

/*
 * External variables
 */


extern bool	after, again, door_stop,
		firstmove, has_hit, kamikaze,
		move_on, running, to_death;

extern char	dir_ch, file_name[], huh[], prbuf[], whoami[],
		l_last_comm, l_last_dir, last_comm, last_dir,
		outbuf[], *r_stones[], runch,
		take, *tr_name[];

extern int	food_left, hungry_state,
		level, max_hit, max_level, mpos,
		no_command, no_food, no_move, purse,
		vf_hit, wizard;

extern unsigned int	seed;

extern WINDOW	*hw;

extern coord oldpos;


extern THING	*l_last_pick, *last_pick;




typedef struct {
    char	*st_name;
    int		st_value;
} STONE;


#endif /* _ROGUE14_ROGUE_H_ */
