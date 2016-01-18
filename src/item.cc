#include <string>

using namespace std;

#include "io.h"
#include "armor.h"

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

void Item::set_nickname(std::string const& new_value) {
  this->nickname = new_value;
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

string const& Item::get_nickname() const {
  return this->nickname;
}

bool Item::is_magic() const {
  switch (this->o_type) {
    case ARMOR:   return static_cast<bool>(this->o_flags & ISPROT) ||
                   this->o_arm != armor_ac(static_cast<armor_t>(this->o_which));

    case WEAPON:  return this->o_hplus != 0 || this->o_dplus != 0;
    case AMMO:    return this->o_hplus != 0 || this->o_dplus != 0;

    case POTION:  return true;
    case SCROLL:  return true;
    case STICK:   return true;
    case RING:    return true;
    case AMULET:  return true;

    default:      return false;
  }
}


