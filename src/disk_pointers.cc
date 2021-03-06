#include <string>

#include "io.h"
#include "amulet.h"
#include "potions.h"
#include "scrolls.h"
#include "food.h"
#include "weapons.h"
#include "armor.h"
#include "rings.h"
#include "wand.h"
#include "error_handling.h"

#include "disk.h"

using namespace std;

template<>
void Disk::save<Item>(tag_type tag, Item* element, std::ofstream& data) {
  save_tag(tag, data);
  if (element == nullptr) {
    save(tag, 0, data);

  } else {
    save(tag, 1, data);
    save(tag, element->o_type, data);
    element->save(data);
  }
}

template<>
bool Disk::load<Item>(tag_type tag, Item*& element, std::ifstream& data) {
  if (!load_tag(tag, data)) { return false; }

  int null_checker;
  load(tag, null_checker, data);
  if (null_checker == 0) {
    return true;
  }

  int type;
  load(tag, type, data);
  switch (type) {
    case IO::Potion: element = new class Potion(data); break;
    case IO::Scroll: element = new class Scroll(data); break;
    case IO::Food:   element = new class Food(data); break;
    case IO::Amulet: element = new class Amulet(data); break;
    case IO::Ammo:   element = new class Weapon(data); break;
    case IO::Weapon: element = new class Weapon(data); break;
    case IO::Armor:  element = new class Armor(data); break;
    case IO::Ring:   element = new class Ring(data); break;
    case IO::Wand:   element = new class Wand(data); break;

    default: error("Unknown item");
  }

  return true;
}
