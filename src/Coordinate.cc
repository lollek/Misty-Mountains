#include "coord.h"

Coordinate::Coordinate(int x, int y)
  : x(x), y(y)
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
