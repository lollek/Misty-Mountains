#include "Coordinate.h"

Coordinate::Coordinate(int _x, int _y)
  : x(_x), y(_y)
{}

bool Coordinate::operator==(Coordinate const& other) const {
  return this->x == other.x &&
         this->y == other.y;
}

int Coordinate::get_x() const {
  return this->x;
}

int Coordinate::get_y() const {
  return this->y;
}
