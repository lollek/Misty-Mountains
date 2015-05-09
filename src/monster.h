#ifndef _ROGUE14_MONSTER_H_
#define _ROGUE14_MONSTER_H_

#include <stdbool.h>

#include "rogue.h"

/* Array containing information on all the various types of monsters */
struct monster {
    char *m_name;			/* What to call the monster */
    int m_carry;			/* Probability of carrying something */
    short m_flags;			/* things about the monster */
    struct stats m_stats;		/* Initial stats */
};

extern struct monster monsters[];
THING *mlist;  /* List of monsters on the level */

bool monster_is_blind(THING *mon);
bool monster_is_cancelled(THING *mon);
bool monster_is_confused(THING *mon);
bool monster_is_confusing(THING *mon);
bool monster_is_cursed(THING *mon);
bool monster_is_found(THING *mon);
bool monster_is_hallucinating(THING *mon);
bool monster_is_invisible(THING *mon);
bool monster_is_levitating(THING *mon);
bool monster_is_true_seeing(THING *mon);

void monster_set_blind(THING *mon);
void monster_set_cancelled(THING *mon);
void monster_set_confused(THING *mon);
void monster_set_confusing(THING *mon);
void monster_set_cursed(THING *mon);
void monster_set_found(THING *mon);
void monster_set_hallucinating(THING *mon);
void monster_set_invisible(THING *mon);
void monster_set_levitating(THING *mon);
void monster_set_true_seeing(THING *mon);
void monster_become_stuck(THING *monster);
void monster_become_held(THING *monster);

void monster_remove_blind(THING *mon);
void monster_remove_cancelled(THING *mon);
void monster_remove_confused(THING *mon);
void monster_remove_confusing(THING *mon);
void monster_remove_cursed(THING *mon);
void monster_remove_found(THING *mon);
void monster_remove_hallucinating(THING *mon);
void monster_remove_invisible(THING *mon);
void monster_remove_levitating(THING *mon);
void monster_remove_true_seeing(THING *mon);
void monster_remove_held(THING *mon);

/* Pick a monster to show up.  The lower the level, the meaner the monster. */
char monster_random(bool wander);

/* Pick a new monster and add it to the monster list */
void monster_new(THING *tp, char type, coord *cp);

/* Create a new wandering monster and aim it at the player */
void monster_new_random_wanderer(void);

/* What to do when the hero steps next to a monster */
THING *monster_notice_player(int y, int x);

/* Give a pack to a monster if it deserves one */
void monster_give_pack(THING *tp);

/* See if a creature save against something */
int monster_save_throw(int which, THING *tp);

/* Make monster start running (towards hero?) */
void monster_start_running(coord *runner);

/* Find proper destination for monster */
coord *monster_destination(THING *tp);

/* Called to put a monster to death */
void monster_on_death(THING *tp, bool pr);

/* Remove a monster from the screen */
void monster_remove_from_screen(coord *mp, THING *tp, bool waskill);

bool monster_is_dead(THING *monster);

void monster_teleport(THING *monster, coord *destination);

/** monster_chase.c **/
bool monster_chase(THING *tp); /* Make a monster chase */


#endif /* _ROGUE14_MONSTER_H_ */
