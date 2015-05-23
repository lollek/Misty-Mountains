#ifndef _ROGUE14_TRAPS_H_
#define _ROGUE14_TRAPS_H_

#include "coord.h"
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

/* Trap victim with trap at position (since it has not yet moved there).
 * if player, victim should be NULL */
enum trap_t be_trapped(THING *victim, coord *trap_coord);
extern const char *trap_names[NTRAPS];


#endif /* _ROGUE14_TRAPS_H_ */
