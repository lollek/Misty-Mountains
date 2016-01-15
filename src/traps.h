#ifndef ROGUE14_TRAPS_H
#define ROGUE14_TRAPS_H

#include "Coordinate.h"
#include "things.h"

/* Trap types */
enum trap_t
{
  T_DOOR  = 0,
  T_ARROW = 1,
  T_SLEEP = 2,
  T_BEAR  = 3,
  T_TELEP = 4,
  T_DART  = 5,
  T_RUST  = 6,
  T_MYST  = 7,
  NTRAPS
};

extern char const* trap_names[NTRAPS];

/* Trap victim with trap at position (since it has not yet moved there).
 * if player, victim should be NULL */
enum trap_t trap_spring(monster* victim, Coordinate* trap_coord);


#endif /* ROGUE14_TRAPS_H */
