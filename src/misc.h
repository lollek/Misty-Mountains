#ifndef _ROGUE14_MISC_H_
#define _ROGUE14_MISC_H_

#include <stdbool.h>

#include "rogue.h"

int rnd(int range); /* Pick a very random number. */
int roll(int number, int sides); /* Roll a number of dice */
void look(bool wakeup); /* A quick glance all around the player */
void erase_lamp(coord *pos, struct room *rp); /* Erase the area shown by a lamp in a dark room. */
bool show_floor(void); /* Should we show the floor in her room at this time? */
THING *find_obj(int y, int x); /* Find the unclaimed object at y, x */
bool eat(void); /* She wants to eat something, so let her try */
void aggravate(void); /* Aggravate all the monsters on this level */
const char *vowelstr(const char *str); /* For printfs: if string starts with a vowel, return "n" for an "an" */
bool get_dir(void); /* Set up the direction co_ordinate for use in varios "prefix" commands */
int sign(int nm); /* Return the sign of the number */
int spread(int nm); /* Give a spread around a given number (+/- 20%) */
void call_it(struct obj_info *info); /* Call an object something after use */
char rnd_thing(void); /* Pick a random thing appropriate for this level */
bool is_magic(THING *obj);   /* Returns true if an object radiates magic */
bool seen_stairs(void);          /* Return true if the player has seen the stairs */
bool turn_see(bool turn_off);/* Put on or off seeing monsters on this level */
void invis_on(void);         /* Turn on the ability to see invisible */
void strucpy(char *s1, const char *s2, int len); /* Copy string using unctrl for things */
void waste_time(int rounds);
void set_oldch(THING *tp, coord *cp); /* Set oldch for a monster */
bool see_monst(THING *mp);  /* Can player see the monster? */
struct room *roomin(coord *cp); /* Find what room some coords are in, NULL means no room */
bool diag_ok(coord *sp, coord *ep); /* Check if move is legal if diagonal */
bool cansee(int y, int x); /* True if player can see coord */
/** Dist
 * Calculate the "distance" between to points.  Actually,
 * this calculates d^2, not d, but that's good enough for
 * our purposes, since it's only used comparitively. */
int dist(int y1, int x1, int y2, int x2);
#define dist_cp(_1, _2) dist((_1)->y, (_1)->x, (_2)->y, (_2)->x)
char *set_mname(THING *tp); /* return the monster name for the given monster */
const char *pick_color(const char *col); /* Return given color (or random if hallucinating) */
char floor_ch(void); /* Floor-type at hero's position */
char floor_at(void); /* Return the character at hero's position */
void reset_last(void); /* Reset the last command when the current one is aborted */
bool player_has_ring_with_ability(int ability);

#endif /* _ROGUE14_MISC_H_ */
