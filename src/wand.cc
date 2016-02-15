#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

#include "options.h"
#include "disk.h"
#include "error_handling.h"
#include "game.h"
#include "colors.h"
#include "coordinate.h"
#include "fight.h"
#include "io.h"
#include "item.h"
#include "level.h"
#include "magic.h"
#include "misc.h"
#include "monster.h"
#include "options.h"
#include "os.h"
#include "player.h"
#include "rogue.h"
#include "weapons.h"

#include "wand.h"

using namespace std;

vector<string>* Wand::materials;
vector<string>* Wand::guesses;
vector<bool>*   Wand::known;

static Wand::Type random_wand_type() {
  vector<Wand::Type> potential_wands;

  switch (Game::current_level) {
    default:

      [[clang::fallthrough]];
    case 20:
      potential_wands.push_back(Wand::Polymorph);
      potential_wands.push_back(Wand::TeleportAway);
      potential_wands.push_back(Wand::TeleportTo);
      potential_wands.push_back(Wand::Cancellation);
      potential_wands.push_back(Wand::DrainLife);

      [[clang::fallthrough]];
    case 15: case 16: case 17: case 18: case 19:
      potential_wands.push_back(Wand::ElectricBolt);
      potential_wands.push_back(Wand::FireBolt);
      potential_wands.push_back(Wand::ColdBolt);

      [[clang::fallthrough]];
    case 11: case 12: case 13: case 14:
    case  5: case  6: case  7: case  8: case  9:
    case  1: case  2: case  3: case  4:
      potential_wands.push_back(Wand::HasteMonster);
      potential_wands.push_back(Wand::SlowMonster);
      potential_wands.push_back(Wand::Light);
      potential_wands.push_back(Wand::MagicMissile);
      potential_wands.push_back(Wand::InvisibleOther);
  }

  return potential_wands.at(os_rand_range(potential_wands.size()));
}

bool Wand::is_magic() const {
  return true;
}


string Wand::name(Wand::Type subtype) {
  switch (subtype) {
    case HasteMonster:   return "haste monster";
    case SlowMonster:    return "slow monster";
    case InvisibleOther: return "invisible other";
    case Light:          return "light";
    case MagicMissile:   return "magic missile";
    case ElectricBolt:   return "electric bolt";
    case FireBolt:       return "fire bolt";
    case ColdBolt:       return "cold bolt";
    case Cancellation:   return "cancellation";
    case Polymorph:      return "polymorph";
    case TeleportTo:     return "teleport to";
    case TeleportAway:   return "teleport away";
    case DrainLife:      return "drain life";

    case NWANDS:    error("Unknown type NWANDS");
  };
}

int Wand::worth(Wand::Type subtype) {
  switch (subtype) {
    case HasteMonster:   return 0;
    case SlowMonster:    return 500;
    case InvisibleOther: return 0;
    case Light:          return 200;
    case MagicMissile:   return 200;
    case ElectricBolt:   return 600;
    case FireBolt:       return 600;
    case ColdBolt:       return 600;
    case Cancellation:   return 500;
    case Polymorph:      return 400;
    case TeleportTo:     return 350;
    case TeleportAway:   return 350;
    case DrainLife:      return 600;

    case NWANDS:    error("Unknown type NWANDS");
  };
}

string& Wand::guess(Wand::Type subtype) {
  return guesses->at(static_cast<size_t>(subtype));
}

bool Wand::is_known(Wand::Type subtype) {
  return known->at(static_cast<size_t>(subtype));
}

void Wand::set_known(Wand::Type subtype) {
  known->at(static_cast<size_t>(subtype)) = true;
}

void Wand::init_wands() {
  materials = new vector<string>;
  known = new vector<bool>(Wand::NWANDS, false);
  guesses = new vector<string>(Wand::NWANDS, "");

  vector<string> possible_material = {
    /* Wood */
    "avocado wood", "balsa", "bamboo", "banyan", "birch", "cedar", "cherry",
    "cinnibar", "cypress", "dogwood", "driftwood", "ebony", "elm", "eucalyptus",
    "fall", "hemlock", "holly", "ironwood", "kukui wood", "mahogany",
    "manzanita", "maple", "oaken", "persimmon wood", "pecan", "pine", "poplar",
    "redwood", "rosewood", "spruce", "teak", "walnut", "zebrawood",

    /* Metal */
    "aluminum", "beryllium", "bone", "brass", "bronze", "copper", "electrum",
    "gold", "iron", "lead", "magnesium", "mercury", "nickel", "pewter",
    "platinum", "steel", "silver", "silicon", "tin", "titanium", "tungsten",
    "zinc",
  };

  while (materials->size() < static_cast<size_t>(Wand::NWANDS)) {
    string new_material = possible_material.at(os_rand_range(possible_material.size()));
    if (find(materials->cbegin(), materials->cend(), new_material) ==
        materials->cend()) {
      materials->push_back(new_material);
    }
  }

  // Run some checks
  if (materials->size() != static_cast<size_t>(Wand::NWANDS)) {
    error("Wand init: wrong number of materials");
  } else if (known->size() != static_cast<size_t>(Wand::NWANDS)) {
    error("Wand init: wrong number of knowledge");
  } else if (guesses->size() != static_cast<size_t>(Wand::NWANDS)) {
    error("Wand init: wrong number of guesses");
  }
}

void Wand::save_wands(std::ofstream& data) {
  Disk::save_tag(TAG_WANDS, data);
  Disk::save(TAG_MATERIALS, materials, data);
  Disk::save(TAG_KNOWN, known, data);
  Disk::save(TAG_GUESSES, guesses, data);
}

void Wand::load_wands(std::ifstream& data) {
  if (!Disk::load_tag(TAG_WANDS, data))            { error("No wands found"); }
  if (!Disk::load(TAG_MATERIALS, materials, data)) { error("Wand tag error 1"); }
  if (!Disk::load(TAG_KNOWN, known, data))         { error("Wand tag error 2"); }
  if (!Disk::load(TAG_GUESSES, guesses, data))     { error("Wand tag error 3"); }
}



void Wand::free_wands() {
  delete materials;
  materials = nullptr;

  delete guesses;
  guesses = nullptr;

  delete known;
  known = nullptr;
}

string const& Wand::material(Wand::Type subtype) {
  return materials->at(static_cast<size_t>(subtype));
}

void Wand::set_identified() {
  set_known(subtype);
  identified = true;
}

bool Wand::is_identified() const {
  return identified;
}


Wand::~Wand() {}

Wand::Wand(std::ifstream& data) {
  load(data);
}

Wand::Wand() : Wand(random_wand_type()) {}

Wand::Wand(Wand::Type subtype_) : Item(), identified(false) {
  o_type = IO::Wand;
  set_attack_damage({1, 1});
  set_throw_damage({1, 1});
  set_armor(11);
  o_count = 1;
  o_which = subtype_;
  subtype = subtype_;

  if (subtype == Wand::Light) {
    charges = os_rand_range(10) + 10;
  } else {
    charges = os_rand_range(5) + 3;
  }
}

Wand* Wand::clone() const {
  return new Wand(*this);
}

string Wand::get_material() const {
  return Wand::material(subtype);
}

string Wand::get_description() const {
  stringstream os;

  bool is_known = Wand::is_known(subtype);
  string const& guess = Wand::guess(subtype);

  os
    << "a"
    << vowelstr(Wand::material(subtype))
    << " "
    << Wand::material(subtype)
    << " wand";

  if (is_known) {
    os << " of " << Wand::name(subtype);

    if (identified) {
      os << " [" << charges << " charges]";
    } else {
      os << " [? charges]";
    }

  } else if (!guess.empty()) {
    os << " {" << guess << "}";
  }

  return os.str();
}

int Wand::get_charges() const {
  return charges;
}

void Wand::set_charges(int amount) {
  charges = amount;
}

void Wand::modify_charges(int amount) {
  charges += amount;
}

void Wand::save(std::ofstream& data) const {
  Item::save(data);
  static_assert(sizeof(Wand::Type) == sizeof(int), "Wrong Wand::Type size");
  Disk::save(TAG_WANDS, static_cast<int>(subtype), data);
  Disk::save(TAG_WANDS, identified, data);
  Disk::save(TAG_WANDS, charges, data);
}

bool Wand::load(std::ifstream& data) {
  if (!Item::load(data) ||
      !Disk::load(TAG_WANDS, reinterpret_cast<int&>(subtype), data) ||
      !Disk::load(TAG_WANDS, identified, data) ||
      !Disk::load(TAG_WANDS, charges, data)) {
    return false;
  }
  return true;
}


int Wand::get_base_value() const {
  return worth(subtype);
}

int Wand::get_value() const {
  int value = get_base_value();
  if (is_identified()) {
    value *= get_charges() * 1.05;
  } else {
    value /= 2;
  }
  value *= o_count;
  return value;
}

bool Wand::is_stackable() const {
  return false;
}

bool Wand::autopickup() const {
  return option_autopickup(o_type);
}
