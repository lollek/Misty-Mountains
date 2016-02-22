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

  bool is_dark;          // Room is dark
  bool is_gone;          // No room here, just corridor
  bool is_maze;          // No room here, just maze
};

