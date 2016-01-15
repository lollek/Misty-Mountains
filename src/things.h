#pragma once

#include <list>
#include <vector>
#include <string>

#include "Coordinate.h"
#include "rooms.h"
#include "damage.h"
#include "item.h"

#define MAXATTACKS 3

/* Stuff about objects */
struct obj_info {
  std::string oi_name;
  size_t      oi_prob;
  size_t      oi_worth;
  std::string oi_guess;
  bool        oi_know;
};

/* Structure describing a fighting being */
struct stats {
  int           s_str;             /* Strength */
  int           s_exp;             /* Experience */
  int           s_lvl;             /* level of mastery */
  int           s_arm;             /* Armor class */
  int           s_hpt;             /* Hit points */
  damage        s_dmg[MAXATTACKS]; /* String describing damage done */
  int           s_maxhp;           /* Max hit points */
};

struct monster {
  stats              t_stats;   /* Physical description */
  Coordinate         t_pos;     /* Position */
  Coordinate         t_dest;    /* Where it is running to */
  room*              t_room;    /* Current room for thing */
  std::list<Item*>   t_pack;    /* What the thing is carrying */

  int                t_flags;   /* State word */
  char               t_type;    /* What it is */
  char               t_disguise;/* What mimic looks like */
  char               t_oldch;   /* Character that was where it was */
  bool               t_turn;    /* If slowed, is it a turn to move */
  int                t_reserved;
};


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

#define GOLDCALC	(os_rand_range(50 + 10 * level) + 2)

extern std::vector<obj_info> things;

/* Return the name of something as it would appear in an inventory. */
char* inv_name(char* buf, Item const* item, bool drop);

/* Put something down */
bool drop(void);

/* Return a new thing */
Item* new_thing(void);
Item* new_amulet(void);
Item* new_food(int which);

/* Pick an item out of a list of nitems possible objects */
size_t pick_one(std::vector<obj_info>& start, size_t nitems);

/* list what the player has discovered in this game of a certain type */
void discovered(void);
