#include "tiles.h"

Tile::Tile()
  : type(Shadow), is_passage(false), is_discovered(false), is_real(true),
    trap_type(Trap::NTRAPS), monster(nullptr)
{}

Tile::~Tile() {}
