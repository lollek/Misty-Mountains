#ifndef _ROGUE14_MISC_H_
#define _ROGUE14_MISC_H_

#include <stdbool.h>

#include "rooms.h"
#include "things.h"

int roll(int number, int sides); /* Roll a number of dice */
void look(bool wakeup);          /* A quick glance all around the player */

/* Erase the area shown by a lamp in a dark room. */
void erase_lamp(coord const* pos, struct room const* room);

THING* find_obj(int y, int x); /* Find the unclaimed object at y, x */

void aggravate(void);          /* Aggravate all the monsters on this level */
char const* vowelstr(char const *str); /* Return "n" if vowel else "" */

/* Set up the direction co_ordinate for use in varios "prefix" commands */
coord const* get_dir(void);

int sign(int nm);              /* Return the sign of the number */
int spread(int nm);            /* Give a spread around a given number (+/- 20%) */

void call_it(char const* what, struct obj_info *info); /* Call object something */

char rnd_thing(void);         /* Pick a random thing appropriate for this level */
bool is_magic(THING const* obj);   /* Returns true if an object radiates magic */
bool seen_stairs(void);        /* Return true if the player has seen the stairs */
void invis_on(void);         /* Turn on the ability to see invisible */

/* Copy string using unctrl for things */
void strucpy(char* s1, char const* s2, int len);

void waste_time(int rounds);
void set_oldch(THING* tp, coord* cp); /* Set oldch for a monster */

struct room* roomin(coord* cp); /* Find what room some coords are in, NULL means no room */
bool diag_ok(coord const* sp, coord const* ep); /* Check if move is legal if diagonal */
bool cansee(int y, int x); /* True if player can see coord */
/** Dist
 * Calculate the "distance" between to points.  Actually,
 * this calculates d^2, not d, but that's good enough for
 * our purposes, since it's only used comparitively. */
int dist(int y1, int x1, int y2, int x2);
#define dist_cp(_1, _2) dist((_1)->y, (_1)->x, (_2)->y, (_2)->x)

const char *pick_color(const char *col); /* Return given color (or random if hallucinating) */
char floor_ch(void); /* Floor-type at hero's position */
char floor_at(void); /* Return the character at hero's position */
bool player_has_ring_with_ability(int ability);

/* Pick a random position around the give (y, x) coordinates */
bool fallpos(coord const* pos, coord* newpos);


#endif /* _ROGUE14_MISC_H_ */
