#ifndef _ROGUE14_TRAPS_H_
#define _ROGUE14_TRAPS_H_

#include "rogue.h" /* THING, coord */

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

enum trap_t be_trapped(coord *tc);
extern const char *trap_names[NTRAPS];


#endif /* _ROGUE14_TRAPS_H_ */
