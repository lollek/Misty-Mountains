#include <string>

using namespace std;

#include "io.h"
#include "armor.h"

#include "item.h"

Item::~Item() {}

void Item::set_position(Coordinate const& new_value) {
  position_on_screen = new_value;
}

void Item::set_x(int new_value) {
  position_on_screen.x = new_value;
}

void Item::set_y(int new_value) {
  position_on_screen.y = new_value;
}

void Item::set_nickname(std::string const& new_value) {
  nickname = new_value;
}



Coordinate const& Item::get_position() const {
  return position_on_screen;
}

int Item::get_x() const {
  return position_on_screen.x;
}

int Item::get_y() const {
  return position_on_screen.y;
}

string const& Item::get_nickname() const {
  return nickname;
}

int Item::get_type() const {
  return o_type;
}

void Item::set_attack_damage(damage const& dmg) {
  attack_damage = dmg;
}

void Item::set_throw_damage(damage const& dmg) {
  throw_damage = dmg;
}

damage const& Item::get_attack_damage() const {
  return attack_damage;
}

damage const& Item::get_throw_damage() const {
  return throw_damage;
}

int Item::get_hit_plus() const {
  return hit_plus;
}

int Item::get_damage_plus() const {
  return damage_plus;
}

int Item::get_armor() const {
  return armor;
}

void Item::modify_hit_plus(int amount) {
  hit_plus += amount;
}

void Item::modify_damage_plus(int amount) {
  damage_plus += amount;
}

void Item::modify_armor(int amount) {
  armor += amount;
}

void Item::set_hit_plus(int value) {
  hit_plus = value;
}

void Item::set_damage_plus(int value) {
  damage_plus = value;
}

void Item::set_armor(int value) {
  armor = value;
}

