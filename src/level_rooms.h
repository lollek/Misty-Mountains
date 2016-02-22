#pragma once

#include "coordinate.h"

struct room {
  room() = default;
  room(room const&) = default;
  ~room() = default;

  room& operator=(room const&) = default;
  room& operator=(room&&) = default;

  Coordinate r_pos;      // Upper left corner
  Coordinate r_max;      // Size of room
  Coordinate r_gold;     // Where the gold is
  int r_goldval;         // How much the gold is worth
  int r_flags;           // info about the room
  int r_nexits;          // Number of exits
  Coordinate r_exit[12]; // Where the exits are
};

// flags for rooms
#define ISDARK	0000001		// room is dark
#define ISGONE	0000002		// room is gone (a corridor)
#define ISMAZE	0000004		// room is gone (a corridor)
