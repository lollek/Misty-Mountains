#include <string>

#include "error_handling.h"
#include "io.h"
#include "item/amulet.h"
#include "item/armor.h"
#include "item/food.h"
#include "item/potions.h"
#include "item/rings.h"
#include "item/scrolls.h"
#include "item/wand.h"
#include "item/weapons.h"

#include "disk.h"

using namespace std;

template <>
void Disk::save<Item>(tag_type tag, Item* element, std::ostream& data) {
  save_tag(tag, data);
  if (element == nullptr) {
    save(tag, 0, data);

  } else {
    save(tag, 1, data);
    save(tag, element->o_type, data);
    element->save(data);
  }
}

template <>
bool Disk::load<Item>(tag_type tag, Item*& element, std::istream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }

  int null_checker;
  if (!load(tag, null_checker, data)) {
    return false;
  }

  if (null_checker == 0) {
    return true;
  }

  int type;
  load(tag, type, data);
  switch (type) {
    case IO::Potion: element = new class Potion(data); break;
    case IO::Scroll: element = new class Scroll(data); break;
    case IO::Food: element = new class Food(data); break;
    case IO::Amulet: element = new class Amulet(data); break;
    case IO::Ammo: element = new class Weapon(data); break;
    case IO::Weapon: element = new class Weapon(data); break;
    case IO::Armor: element = new class Armor(data); break;
    case IO::Ring: element = new class Ring(data); break;
    case IO::Wand: element = new class Wand(data); break;

    default: error("Unknown item");
  }

  return true;
}
