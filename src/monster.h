#pragma once

#include <string>
#include <list>

#include "level_rooms.h"
#include "Coordinate.h"
#include "item.h"
#include "rogue.h"

#define MAXATTACKS 3


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

class Monster {
public:

  virtual int get_armor() const;
  virtual ~Monster() = default;

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

private:
};


extern std::list<Monster*> monster_list;  /* List of monsters on the level */

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





/* Variables, TODO: Remove these */
extern int    monster_flytrap_hit;

/* Status getters */
static inline bool monster_is_blind(Monster const* mon)
{ return mon->t_flags & ISBLIND; }
static inline bool monster_is_cancelled(Monster const* mon)
{ return mon->t_flags & ISCANC; }
static inline bool monster_is_confused(Monster const* mon)
{ return mon->t_flags & ISHUH; }
static inline bool monster_is_confusing(Monster const* mon)
{ return mon->t_flags & CANHUH; }
static inline bool monster_is_found(Monster const* mon)
{ return mon->t_flags & ISFOUND; }
static inline bool monster_is_hallucinating(Monster const* mon)
{ return mon->t_flags & ISHALU; }
static inline bool monster_is_invisible(Monster const* mon)
{ return mon->t_flags & ISINVIS; }
static inline bool monster_is_levitating(Monster const* mon)
{ return mon->t_flags & ISLEVIT; }
static inline bool monster_is_true_seeing(Monster const* mon)
{ return mon->t_flags & CANSEE; }
static inline bool monster_is_held(Monster const* mon)
{ return mon->t_flags & ISHELD; }
static inline bool monster_is_stuck(Monster const* mon)
{ return mon->t_flags & ISSTUCK; }
static inline bool monster_is_chasing(Monster const* mon)
{ return mon->t_flags & ISRUN; }
static inline bool monster_is_mean(Monster const* mon)
{ return mon->t_flags & ISMEAN; }
static inline bool monster_is_greedy(Monster const* mon)
{ return mon->t_flags & ISGREED; }
static inline bool monster_is_players_target(Monster const* mon)
{ return mon->t_flags & ISTARGET;}
static inline bool monster_is_slow(Monster const* mon)
{ return mon->t_flags & ISSLOW; }
static inline bool monster_is_hasted(Monster const* mon)
{ return mon->t_flags & ISHASTE; }
static inline bool monster_is_flying(Monster const* mon)
{ return mon->t_flags & ISFLY; }

/* Status setters */
static inline void monster_set_blind(Monster* mon)
{ mon->t_flags |= ISBLIND; }
static inline void monster_set_cancelled(Monster* mon)
{ mon->t_flags |= ISCANC; }
static inline void monster_set_confused(Monster* mon)
{ mon->t_flags |= ISHUH; }
static inline void monster_set_confusing(Monster* mon)
{ mon->t_flags |= CANHUH; }
static inline void monster_set_found(Monster* mon)
{ mon->t_flags |= ISFOUND; }
static inline void monster_set_hallucinating(Monster* mon)
{ mon->t_flags |= ISHALU; }
static inline void monster_set_levitating(Monster* mon)
{ mon->t_flags |= ISLEVIT; }
static inline void monster_set_true_seeing(Monster* mon)
{ mon->t_flags |= CANSEE; }
static inline void monster_become_stuck(Monster* mon)
{ mon->t_flags |= ISSTUCK; }
void monster_set_invisible(Monster* mon);
void monster_become_held(Monster* monster);

/* Status unsetters */
static inline void monster_remove_blind(Monster* mon)
{ mon->t_flags &= ~ISBLIND; }
static inline void monster_remove_cancelled(Monster* mon)
{ mon->t_flags &= ~ISCANC; }
static inline void monster_remove_confused(Monster* mon)
{ mon->t_flags &= ~ISHUH; }
static inline void monster_remove_confusing(Monster* mon)
{ mon->t_flags &= ~CANHUH; }
static inline void monster_remove_found(Monster* mon)
{ mon->t_flags &= ~ISFOUND; }
static inline void monster_remove_hallucinating(Monster* mon)
{ mon->t_flags &= ~ISHALU; }
static inline void monster_remove_invisible(Monster* mon)
{ mon->t_flags &= ~ISINVIS; }
static inline void monster_remove_levitating(Monster* mon)
{ mon->t_flags &= ~ISLEVIT; }
static inline void monster_remove_true_seeing(Monster* mon)
{ mon->t_flags &= ~CANSEE; }
static inline void monster_remove_held(Monster* mon)
{ mon->t_flags &= ~ISHELD; }

/* Pick a monster to show up.  The lower the level, the meaner the monster. */
char monster_random(bool wander);

/* Pick a new monster and add it to the monster list */
void monster_new(Monster* tp, char type, Coordinate* cp, room* room);

/* Create a new wandering monster and aim it at the player */
void monster_new_random_wanderer(void);

/* What to do when the hero steps next to a monster */
Monster *monster_notice_player(int y, int x);

/* Give a pack to a monster if it deserves one */
void monster_give_pack(Monster* mon);

/* See if a creature save against something */
int monster_save_throw(int which, Monster const* mon);

/* Make monster start running (towards hero?) */
void monster_start_running(Coordinate const* runner);

/* Called to put a monster to death */
void monster_on_death(Monster* tp, bool pr);

/* Remove a monster from the screen */
void monster_remove_from_screen(Coordinate* mp, Monster* tp, bool waskill);

bool monster_is_dead(Monster const* monster);

void monster_teleport(Monster* monster, Coordinate const* destination);

void monster_do_special_ability(Monster** monster);

char const* monster_name(Monster const* monster, char* buf);
std::string const& monster_name_by_type(char monster_type);
bool monster_seen_by_player(Monster const* monster);

/* Is any monster seen by the player? */
bool monster_is_anyone_seen_by_player(void);
/* Change all monster visuals due to player tripping */
void monster_show_all_as_trippy(void);
/* Make all monsters take their turn */
void monster_move_all(void);
/* Make all monsters start chasing the player */
void monster_aggravate_all(void);
/* Show all monsters as they truly are */
void monster_show_all_hidden(void);
/* Does any monster desire this item? If so, aggro player */
void monster_aggro_all_which_desire_item(Item* item);
/* Hide all invisible monsters */
void monster_hide_all_invisible(void);
/* Show all monsters that the player does not currently sees
 * Return true if there was atleast one, else false */
bool monster_sense_all_hidden(void);
void monster_unsense_all_hidden(void);
/* Print all monsters as they look (or hide, is in disguise) */
void monster_print_all(void);
/* Print a $ where there is a monster with a magic item
 * Returns true if there was atleast one, else false */
bool monster_show_if_magic_inventory(void);

/* Add nearby monsters to the given list. Returns the number of results */
int monster_add_nearby(Monster** nearby_monsters, struct room const* room);

/* Transform the monster into something else */
void monster_polymorph(Monster* monster);

/** monster_chase.c **/
bool monster_chase(Monster* tp); /* Make a monster chase */
