#pragma once

#include <string>

#include "monster.h"

int roll(int number, int sides); /* Roll a number of dice */
void look(bool wakeup);          /* A quick glance all around the player */

/* Erase the area shown by a lamp in a dark room. */
void erase_lamp(Coordinate const* pos, struct room const* room);

std::string vowelstr(std::string const& str); /* Return "n" if vowel else "" */

/* Set up the direction co_ordinate for use in varios "prefix" commands */
Coordinate const* get_dir(void);

int sign(int nm);              /* Return the sign of the number */
int spread(int nm);            /* Give a spread around a given number (+/- 20%) */

void call_it(std::string const& what, struct obj_info *info); /* Call object something */

char rnd_thing(void);         /* Pick a random thing appropriate for this level */

/* Copy string using unctrl for things */
void strucpy(char* s1, char const* s2, size_t len);

bool diag_ok(Coordinate const* sp, Coordinate const* ep); /* Check if move is legal if diagonal */
bool cansee(int y, int x); /* True if player can see Coordinate */
/** Dist
 * Calculate the "distance" between to points.  Actually,
 * this calculates d^2, not d, but that's good enough for
 * our purposes, since it's only used comparitively. */
int dist(int y1, int x1, int y2, int x2);
inline int dist_cp(Coordinate* c1, Coordinate* c2) {
  return dist(c1->y, c1->y, c2->y, c2->y);
}

char floor_ch(void); /* Floor-type at hero's position */
char floor_at(void); /* Return the character at hero's position */

/* Pick a random position around the give (y, x) Coordinate */
bool fallpos(Coordinate const* pos, Coordinate* newpos);
