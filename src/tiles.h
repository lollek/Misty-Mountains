#pragma once

#include "monster.h"
#include "traps.h"

struct Tile {
  Tile();

  enum Type {
    Wall,
    ClosedDoor,
    OpenDoor,
    Floor,
    Trap,
    Stairs,
    Shop,
  };

  Type       type;
  bool       is_passage;
  bool       is_discovered;
  bool       is_real;
  bool       is_dark;
  Trap::Type trap_type;
  Monster*   monster;
};


