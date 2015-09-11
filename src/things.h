#ifndef ROGUE14_THINGS_H
#define ROGUE14_THINGS_H

#include <stdbool.h>

#include "coord.h"

typedef struct item item;
typedef struct monster monster;

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
  struct monster {
    /* Linked list pointers */
    union thing* l_next;
    union thing* l_prev;

    struct stats    t_stats;   /* Physical description */
    coord           t_pos;     /* Position */
    coord*          t_dest;    /* Where it is running to */
    struct room*    t_room;    /* Current room for thing */
    union thing*    t_pack;    /* What the thing is carrying */

    int             t_flags;   /* State word */
    char            t_type;    /* What it is */
    char            t_disguise;/* What mimic looks like */
    char            t_oldch;   /* Character that was where it was */
    bool            t_turn;    /* If slowed, is it a turn to move */
    int             t_reserved;
  } t;

  struct item {
    /* Linked list pointers */
    union thing*  l_next;
    union thing*  l_prev;

    coord         o_pos;                 /* Where it lives on the screen */
    char*         o_label;               /* Label for object */
    int           o_type;                /* What kind of object it is */
    int           o_launch;              /* What you need to launch it */
    int           o_count;               /* count for plural objects */
    int           o_which;               /* Which object of a type it is */
    int           o_hplus;               /* Plusses to hit */
    int           o_dplus;               /* Plusses to damage */
    int           o_arm;                 /* Armor protection */
    int           o_flags;               /* information about objects */
    char          o_packch;              /* What character it is in the pack */
    struct damage o_damage;              /* Damage if used like sword */
    struct damage o_hurldmg;             /* Damage if thrown */
  } o;

} THING;

#define o_charges	o_arm
#define o_goldval	o_arm

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

#define GOLDCALC	(os_rand_range(50 + 10 * level) + 2)

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

#endif /* ROGUE14_THINGS_H */
