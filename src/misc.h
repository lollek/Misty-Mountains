#pragma once

#include <string>

#include "rooms.h"
#include "things.h"

int roll(int number, int sides); /* Roll a number of dice */
void look(bool wakeup);          /* A quick glance all around the player */

/* Erase the area shown by a lamp in a dark room. */
void erase_lamp(Coordinate const* pos, struct room const* room);

item* find_obj(int y, int x); /* Find the unclaimed object at y, x */

std::string vowelstr(std::string const& str); /* Return "n" if vowel else "" */

/* Set up the direction co_ordinate for use in varios "prefix" commands */
Coordinate const* get_dir(void);

int sign(int nm);              /* Return the sign of the number */
int spread(int nm);            /* Give a spread around a given number (+/- 20%) */

void call_it(std::string const& what, struct obj_info *info); /* Call object something */

char rnd_thing(void);         /* Pick a random thing appropriate for this level */
bool is_magic(item const* obj);   /* Returns true if an object radiates magic */
bool seen_stairs(void);        /* Return true if the player has seen the stairs */
void invis_on(void);         /* Turn on the ability to see invisible */

/* Copy string using unctrl for things */
void strucpy(char* s1, char const* s2, int len);

void waste_time(int rounds);
void set_oldch(monster* tp, Coordinate* cp); /* Set oldch for a monster */

struct room* roomin(Coordinate* cp); /* Find what room some Coordinate are in, NULL means no room */
bool diag_ok(Coordinate const* sp, Coordinate const* ep); /* Check if move is legal if diagonal */
bool cansee(int y, int x); /* True if player can see Coordinate */
/** Dist
 * Calculate the "distance" between to points.  Actually,
 * this calculates d^2, not d, but that's good enough for
 * our purposes, since it's only used comparitively. */
int dist(int y1, int x1, int y2, int x2);
#define dist_cp(_1, _2) dist((_1)->y, (_1)->x, (_2)->y, (_2)->x)

const char *pick_color(const char *col); /* Return given color (or random if hallucinating) */
char floor_ch(void); /* Floor-type at hero's position */
char floor_at(void); /* Return the character at hero's position */

/* Pick a random position around the give (y, x) Coordinate */
bool fallpos(Coordinate const* pos, Coordinate* newpos);
