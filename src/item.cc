#include "item.h"

void Item::set_pos(Coordinate const& new_value) {
  this->position_on_screen = new_value;
}

void Item::set_x(int new_value) {
  this->position_on_screen.x = new_value;
}

void Item::set_y(int new_value) {
  this->position_on_screen.y = new_value;
}

Coordinate const& Item::get_pos() const {
  return this->position_on_screen;
}

int Item::get_x() const {
  return this->position_on_screen.x;
}

int Item::get_y() const {
  return this->position_on_screen.y;
}
