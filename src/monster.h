#ifndef ROGUE14_MONSTER_H
#define ROGUE14_MONSTER_H

#include <stdbool.h>

#include "rogue.h"

/* Variables, TODO: Remove these */
extern THING* monster_list;  /* List of monsters on the level */
extern int    monster_flytrap_hit;

bool monsters_save_state(void);

/* Status getters */
static inline bool monster_is_blind(monster const* mon)
{ return mon->t_flags & ISBLIND; }
static inline bool monster_is_cancelled(monster const* mon)
{ return mon->t_flags & ISCANC; }
static inline bool monster_is_confused(monster const* mon)
{ return mon->t_flags & ISHUH; }
static inline bool monster_is_confusing(monster const* mon)
{ return mon->t_flags & CANHUH; }
static inline bool monster_is_found(monster const* mon)
{ return mon->t_flags & ISFOUND; }
static inline bool monster_is_hallucinating(monster const* mon)
{ return mon->t_flags & ISHALU; }
static inline bool monster_is_invisible(monster const* mon)
{ return mon->t_flags & ISINVIS; }
static inline bool monster_is_levitating(monster const* mon)
{ return mon->t_flags & ISLEVIT; }
static inline bool monster_is_true_seeing(monster const* mon)
{ return mon->t_flags & CANSEE; }
static inline bool monster_is_held(monster const* mon)
{ return mon->t_flags & ISHELD; }
static inline bool monster_is_stuck(monster const* mon)
{ return mon->t_flags & ISSTUCK; }
static inline bool monster_is_chasing(monster const* mon)
{ return mon->t_flags & ISRUN; }
static inline bool monster_is_mean(monster const* mon)
{ return mon->t_flags & ISMEAN; }
static inline bool monster_is_greedy(monster const* mon)
{ return mon->t_flags & ISGREED; }
static inline bool monster_is_players_target(monster const* mon)
{ return mon->t_flags & ISTARGET;}
static inline bool monster_is_slow(monster const* mon)
{ return mon->t_flags & ISSLOW; }
static inline bool monster_is_hasted(monster const* mon)
{ return mon->t_flags & ISHASTE; }
static inline bool monster_is_flying(monster const* mon)
{ return mon->t_flags & ISFLY; }

/* Status setters */
static inline void monster_set_blind(monster* mon)
{ mon->t_flags |= ISBLIND; }
static inline void monster_set_cancelled(monster* mon)
{ mon->t_flags |= ISCANC; }
static inline void monster_set_confused(monster* mon)
{ mon->t_flags |= ISHUH; }
static inline void monster_set_confusing(monster* mon)
{ mon->t_flags |= CANHUH; }
static inline void monster_set_found(monster* mon)
{ mon->t_flags |= ISFOUND; }
static inline void monster_set_hallucinating(monster* mon)
{ mon->t_flags |= ISHALU; }
static inline void monster_set_levitating(monster* mon)
{ mon->t_flags |= ISLEVIT; }
static inline void monster_set_true_seeing(monster* mon)
{ mon->t_flags |= CANSEE; }
static inline void monster_become_stuck(monster* mon)
{ mon->t_flags |= ISSTUCK; }
void monster_set_invisible(THING* mon);
void monster_become_held(monster* monster);

/* Status unsetters */
static inline void monster_remove_blind(monster* mon)
{ mon->t_flags &= ~ISBLIND; }
static inline void monster_remove_cancelled(monster* mon)
{ mon->t_flags &= ~ISCANC; }
static inline void monster_remove_confused(monster* mon)
{ mon->t_flags &= ~ISHUH; }
static inline void monster_remove_confusing(monster* mon)
{ mon->t_flags &= ~CANHUH; }
static inline void monster_remove_found(monster* mon)
{ mon->t_flags &= ~ISFOUND; }
static inline void monster_remove_hallucinating(monster* mon)
{ mon->t_flags &= ~ISHALU; }
static inline void monster_remove_invisible(monster* mon)
{ mon->t_flags &= ~ISINVIS; }
static inline void monster_remove_levitating(monster* mon)
{ mon->t_flags &= ~ISLEVIT; }
static inline void monster_remove_true_seeing(monster* mon)
{ mon->t_flags &= ~CANSEE; }
static inline void monster_remove_held(monster* mon)
{ mon->t_flags &= ~ISHELD; }

/* Pick a monster to show up.  The lower the level, the meaner the monster. */
char monster_random(bool wander);

/* Pick a new monster and add it to the monster list */
void monster_new(THING* tp, char type, coord* cp);

/* Create a new wandering monster and aim it at the player */
void monster_new_random_wanderer(void);

/* What to do when the hero steps next to a monster */
THING *monster_notice_player(int y, int x);

/* Give a pack to a monster if it deserves one */
void monster_give_pack(monster* mon);

/* See if a creature save against something */
int monster_save_throw(int which, monster const* mon);

/* Make monster start running (towards hero?) */
void monster_start_running(coord const* runner);

/* Called to put a monster to death */
void monster_on_death(THING* tp, bool pr);

/* Remove a monster from the screen */
void monster_remove_from_screen(coord* mp, THING* tp, bool waskill);

bool monster_is_dead(THING const* monster);

void monster_teleport(THING* monster, coord const* destination);

void monster_do_special_ability(THING** monster);

char const* monster_name(monster const* monster, char* buf);
char const* monster_name_by_type(char monster_type);
bool monster_seen_by_player(monster const* monster);

/** monster_chase.c **/
bool monster_chase(THING* tp); /* Make a monster chase */

#endif /* ROGUE14_MONSTER_H */
