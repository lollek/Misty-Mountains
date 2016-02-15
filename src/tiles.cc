#include "tiles.h"

Tile::Tile()
  : type(Wall), is_passage(false), is_discovered(false), is_real(true),
    is_dark(false), trap_type(Trap::NTRAPS), monster(nullptr)
{}
