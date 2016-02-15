#include "coordinate.h"

Coordinate::Coordinate(int _x, int _y)
  : x(_x), y(_y)
{}

bool Coordinate::operator==(Coordinate const& other) const {
  return x == other.x &&
         y == other.y;
}

bool Coordinate::operator!=(Coordinate const& other) const {
  return !(*this == other);
}
