#pragma once

#include <string>

#include "coordinate.h"
#include "monster.h"

namespace Trap {
enum Type {
  Door  = 0,
  Arrow = 1,
  Sleep = 2,
  Beartrap  = 3,
  Teleport = 4,
  Dart  = 5,
  Rust  = 6,
  Mystery  = 7,
  NTRAPS
};

void init_traps();
void free_traps();

std::string name(Type type);

// Spring a trap on the victim.
// NB! victim can be null after this!
Type spring(Monster** victim, Type type);

// Spring a trap on the player.
Type player(Coordinate const& trap_coord);

// Random trap type
Type random();

}

