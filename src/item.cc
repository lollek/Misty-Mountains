#include <string>

#include "gold.h"
#include "disk.h"
#include "amulet.h"
#include "potions.h"
#include "scrolls.h"
#include "food.h"
#include "weapons.h"
#include "armor.h"
#include "rings.h"
#include "wand.h"
#include "game.h"
#include "os.h"
#include "error_handling.h"
#include "io.h"
#include "armor.h"

#include "item.h"

using namespace std;

Item::~Item() {}

Item::Item()
  : o_type(0), o_launch(0), o_count(1), o_which(0), o_flags(0), o_packch(0),

    position_on_screen(0, 0), nickname(""), attack_damage({1, 2}),
    throw_damage({1, 2}), hit_plus(0), damage_plus(0), armor(0), cursed(false)
{}

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

int Item::get_item_type() const {
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

void Item::set_cursed() {
  cursed = true;
}

void Item::set_not_cursed() {
  cursed = false;
}

bool Item::is_cursed() const {
  return cursed;
}

string Item::name(Item::Type type) {
  switch (type) {
    case Potion: return "potion";
    case Scroll: return "scroll";
    case Food:   return "food";
    case Weapon: return "weapon";
    case Armor:  return "armor";
    case Ring:   return "ring";
    case Wand:   return "wand";
    case Amulet: return "amulet";
    case Gold:   return "gold";
    case NITEMS: error("Unknown type NITEMS");
  }
}

int Item::probability(Item::Type type) {
  static_assert(30 + 30 + 15 + 10 + 10 + 1 + 4 == 100, "Item probability");
  switch (type) {
    case Potion: return 30;
    case Scroll: return 30;
    case Food:   return 15;
    case Weapon: return 10;
    case Armor:  return 10;
    case Ring:   return  1;
    case Wand:   return  4;
    case Amulet: return  0;
    case Gold:   return  0;
    case NITEMS: error("Unknown type NITEMS");
  }
}

static Item::Type random_item_type() {
  int value = os_rand_range(100);
  int end = static_cast<int>(Item::Type::NITEMS);
  for (int i = 0; i < end; ++i) {
    Item::Type type = static_cast<Item::Type>(i);
    int probability = Item::probability(type);

    if (value < probability) {
      return type;

    } else {
      value -= probability;
    }
  }

  error("Error! Sum of probabilities is not 100%");
}

Item* Item::random() {
  // We are kind to the hungry player in this game
  if (Game::levels_without_food > 3) {
    return new class Food();
  }

  switch (random_item_type()) {
    case Potion: return new class Potion();
    case Scroll: return new class Scroll();
    case Food:   return new class Food();
    case Weapon: return new class Weapon(true);
    case Armor:  return new class Armor(true);
    case Ring:   return new class Ring();
    case Wand:   return new class Wand();
    case Amulet: return new class Amulet();
    case Gold:   return new class Gold();
    case NITEMS: error("Unknown type NITEMS");
  }
}

void Item::save(std::ofstream& data) const {
  Disk::save_tag(TAG_ITEM, data);

  Disk::save(TAG_ITEM_PUBLIC, o_type, data);
  Disk::save(TAG_ITEM_PUBLIC, o_launch, data);
  Disk::save(TAG_ITEM_PUBLIC, o_count, data);
  Disk::save(TAG_ITEM_PUBLIC, o_which, data);
  Disk::save(TAG_ITEM_PUBLIC, o_flags, data);
  Disk::save(TAG_ITEM_PUBLIC, o_packch, data);

  Disk::save(TAG_ITEM_PRIVATE, position_on_screen, data);
  Disk::save(TAG_ITEM_PRIVATE, nickname, data);
  Disk::save(TAG_ITEM_PRIVATE, attack_damage, data);
  Disk::save(TAG_ITEM_PRIVATE, throw_damage, data);
  Disk::save(TAG_ITEM_PRIVATE, hit_plus, data);
  Disk::save(TAG_ITEM_PRIVATE, damage_plus, data);
  Disk::save(TAG_ITEM_PRIVATE, armor, data);
  Disk::save(TAG_ITEM_PRIVATE, cursed, data);
}

bool Item::load(std::ifstream& data) {
  if (!Disk::load_tag(TAG_ITEM, data) ||

      !Disk::load(TAG_ITEM_PUBLIC, o_type, data) ||
      !Disk::load(TAG_ITEM_PUBLIC, o_launch, data) ||
      !Disk::load(TAG_ITEM_PUBLIC, o_count, data) ||
      !Disk::load(TAG_ITEM_PUBLIC, o_which, data) ||
      !Disk::load(TAG_ITEM_PUBLIC, o_flags, data) ||
      !Disk::load(TAG_ITEM_PUBLIC, o_packch, data) ||

      !Disk::load(TAG_ITEM_PRIVATE, position_on_screen, data) ||
      !Disk::load(TAG_ITEM_PRIVATE, nickname, data) ||
      !Disk::load(TAG_ITEM_PRIVATE, attack_damage, data) ||
      !Disk::load(TAG_ITEM_PRIVATE, throw_damage, data) ||
      !Disk::load(TAG_ITEM_PRIVATE, hit_plus, data) ||
      !Disk::load(TAG_ITEM_PRIVATE, damage_plus, data) ||
      !Disk::load(TAG_ITEM_PRIVATE, armor, data) ||
      !Disk::load(TAG_ITEM_PRIVATE, cursed, data)) {
    return false;
  }
  return true;
}
