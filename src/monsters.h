#ifndef _ROGUE14_MONSTERS_H_
#define _ROGUE14_MONSTERS_H_

#include "rogue.h"

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

#endif /* _ROGUE14_MONSTERS_H_ */
