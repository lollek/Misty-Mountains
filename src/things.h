#ifndef _ROGUE14_THINGS_H_
#define _ROGUE14_THINGS_H_

#include <stdbool.h>

#include "coord.h"

struct damage
{
  int sides;
  int dices;
};
#define MAXATTACKS 3

/* Stuff about objects */
struct obj_info {
  char* oi_name;
  int   oi_prob;
  int   oi_worth;
  char* oi_guess;
  bool  oi_know;
};

/* Structure describing a fighting being */
struct stats {
  int           s_str;             /* Strength */
  int           s_exp;             /* Experience */
  int           s_lvl;             /* level of mastery */
  int           s_arm;             /* Armor class */
  int           s_hpt;             /* Hit points */
  struct damage s_dmg[MAXATTACKS]; /* String describing damage done */
  int           s_maxhp;           /* Max hit points */
};

/* Structure for monsters and player */
typedef union thing {
  struct {
    /* Linked list pointers */
    union thing* _l_next;
    union thing* _l_prev;

    struct stats _t_stats;   /* Physical description */
    coord        _t_pos;     /* Position */
    coord*       _t_dest;    /* Where it is running to */
    struct room* _t_room;    /* Current room for thing */
    union thing* _t_pack;    /* What the thing is carrying */

    int          _t_flags;   /* State word */
    char         _t_type;    /* What it is */
    char         _t_disguise;/* What mimic looks like */
    char         _t_oldch;   /* Character that was where it was */
    bool         _t_turn;    /* If slowed, is it a turn to move */
    int          _t_reserved;
  } _t;

  struct {
    /* Linked list pointers */
    union thing*  _l_next;
    union thing*  _l_prev;

    coord         _o_pos;                 /* Where it lives on the screen */
    char*         _o_text;                /* What it says if you read it */
    char*         _o_label;               /* Label for object */
    int           _o_type;                /* What kind of object it is */
    int           _o_launch;              /* What you need to launch it */
    int           _o_count;               /* count for plural objects */
    int           _o_which;               /* Which object of a type it is */
    int           _o_hplus;               /* Plusses to hit */
    int           _o_dplus;               /* Plusses to damage */
    int           _o_arm;                 /* Armor protection */
    int           _o_flags;               /* information about objects */
    int           _o_group;               /* group number for this object */
    char          _o_packch;              /* What character it is in the pack */
    struct damage _o_damage[MAXATTACKS];  /* Damage if used like sword */
    struct damage _o_hurldmg[MAXATTACKS]; /* Damage if thrown */
  } _o;

} THING;

#define l_next		_t._l_next
#define l_prev		_t._l_prev
#define t_pos		_t._t_pos
#define t_turn		_t._t_turn
#define t_type		_t._t_type
#define t_disguise	_t._t_disguise
#define t_oldch		_t._t_oldch
#define t_dest		_t._t_dest
#define t_flags		_t._t_flags
#define t_stats		_t._t_stats
#define t_pack		_t._t_pack
#define t_room		_t._t_room
#define t_reserved      _t._t_reserved
#define o_type		_o._o_type
#define o_pos		_o._o_pos
#define o_text		_o._o_text
#define o_launch	_o._o_launch
#define o_packch	_o._o_packch
#define o_damage	_o._o_damage
#define o_hurldmg	_o._o_hurldmg
#define o_count		_o._o_count
#define o_which		_o._o_which
#define o_hplus		_o._o_hplus
#define o_dplus		_o._o_dplus
#define o_arm		_o._o_arm
#define o_charges	o_arm
#define o_goldval	o_arm
#define o_flags		_o._o_flags
#define o_group		_o._o_group
#define o_label		_o._o_label

/* flags for objects */
#define ISCURSED 000001		/* object is cursed */
#define ISKNOW	0000002		/* player knows details about the object */
#define ISMISL	0000004		/* object is a missile type */
#define ISMANY	0000010		/* object comes in groups */
/*	ISFOUND 0000020		...is used for both objects and creatures */
#define ISPROT	0000040		/* armor is permanently protected */

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

#define GOLDCALC	(rnd(50 + 10 * level) + 2)

#define NUMTHINGS 7
extern struct obj_info things[NUMTHINGS];

/* Return the name of something as it would appear in an inventory. */
char* inv_name(char* buf, THING* obj, bool drop);

/* Put something down */
bool drop(void);

/* Return a new thing */
THING* new_thing(void);
THING* new_amulet(void);
THING* new_food(int which);

/* Pick an item out of a list of nitems possible objects */
unsigned pick_one(struct obj_info* start, int nitems);

/* list what the player has discovered in this game of a certain type */
void discovered(void);

#endif /* _ROGUE14_THINGS_H_ */
