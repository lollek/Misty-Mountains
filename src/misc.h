#pragma once

#include <string>

#include "monster.h"

int roll(int number, int sides); /* Roll a number of dice */

std::string vowelstr(std::string const& str); /* Return "n" if vowel else "" */

/* Set up the direction co_ordinate for use in varios "prefix" commands */
Coordinate const* get_dir(void);

int sign(int nm);              /* Return the sign of the number */
int spread(int nm);            /* Give a spread around a given number (+/- 20%) */

char rnd_thing(void);         /* Pick a random thing appropriate for this level */

/** Dist
 * Calculate the "distance" between to points.  Actually,
 * this calculates d^2, not d, but that's good enough for
 * our purposes, since it's only used comparitively. */
int dist(int y1, int x1, int y2, int x2);
inline int dist_cp(Coordinate const* c1, Coordinate const* c2) {
  return dist(c1->y, c1->x, c2->y, c2->x);
}

/* Pick a random position around the give (y, x) Coordinate */
bool fallpos(Coordinate const* pos, Coordinate* newpos);
