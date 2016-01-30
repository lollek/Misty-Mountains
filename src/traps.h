#pragma once

#include <string>

#include "player.h"
#include "coordinate.h"
#include "monster.h"

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

extern std::string const trap_names[NTRAPS];

// Spring a trap on the victim.
// NB! victim can be null after this!
trap_t trap_spring(Monster** victim, Coordinate const& trap_coord);

// Spring a trap on the player.
trap_t trap_player(Coordinate const& trap_coord);
