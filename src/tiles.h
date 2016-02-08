#pragma once

#include "monster.h"
#include "traps.h"

struct Tile {
  Tile();
  ~Tile();

  enum Type {
    Shadow,
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
  size_t     passage_number;
  Trap::Type trap_type;
  Monster*   monster;
};


