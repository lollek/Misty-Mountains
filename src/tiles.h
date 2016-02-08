#pragma once

#include "monster.h"
#include "traps.h"

struct Tile {
  Tile();
  ~Tile();

  enum Type {
    Wall,
    Door,
    Floor,
    Trap,
    Stairs,
  };

  Type       type;
  bool       is_passage;
  bool       is_discovered;
  bool       is_real;
  Trap::Type trap_type;
  Monster*   monster;
};


