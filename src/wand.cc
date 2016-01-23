#include <string>
#include <vector>
#include <sstream>

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
#include "pack.h"
#include "player.h"
#include "rogue.h"
#include "things.h"
#include "weapons.h"

#include "wand.h"

using namespace std;

vector<string> Wand::materials;
vector<string> Wand::guesses;
vector<bool>   Wand::known;

static Wand::Type random_wand_type() {
  int value = os_rand_range(100);

  int end = static_cast<int>(Wand::Type::NWANDS);
  for (int i = 0; i < end; ++i) {
    Wand::Type type = static_cast<Wand::Type>(i);
    int probability = Wand::probability(type);

    if (value < probability) {
      return type;

    } else {
      value -= probability;
    }
  }

  error("Error! Sum of probabilities is not 100%");
}



string Wand::name(Wand::Type subtype) {
  switch (subtype) {
  case LIGHT:     return "light";
  case INVIS:     return "invisibility";
  case ELECT:     return "lightning";
  case FIRE:      return "fire";
  case COLD:      return "cold";
  case POLYMORPH: return "polymorph";
  case MISSILE:   return "magic missile";
  case HASTE_M:   return "haste monster";
  case SLOW_M:    return "slow monster";
  case DRAIN:     return "drain life";
  case NOP:       return "nothing";
  case TELAWAY:   return "teleport away";
  case TELTO:     return "teleport to";
  case CANCEL:    return "cancellation";
  case NWANDS:    error("Unknown type NWANDS");
  };
}

int Wand::probability(Wand::Type subtype) {
  switch (subtype) {
  case LIGHT:     return 12;
  case INVIS:     return  6;
  case ELECT:     return  3;
  case FIRE:      return  3;
  case COLD:      return  3;
  case POLYMORPH: return 15;
  case MISSILE:   return 10;
  case HASTE_M:   return 10;
  case SLOW_M:    return 11;
  case DRAIN:     return  9;
  case NOP:       return  1;
  case TELAWAY:   return  6;
  case TELTO:     return  6;
  case CANCEL:    return  5;
  case NWANDS:    error("Unknown type NWANDS");
  };
}

int Wand::worth(Wand::Type subtype) {
  switch (subtype) {
  case LIGHT:     return 250;
  case INVIS:     return   5;
  case ELECT:     return 330;
  case FIRE:      return 330;
  case COLD:      return 330;
  case POLYMORPH: return 310;
  case MISSILE:   return 170;
  case HASTE_M:   return   5;
  case SLOW_M:    return 350;
  case DRAIN:     return 300;
  case NOP:       return   5;
  case TELAWAY:   return 340;
  case TELTO:     return  50;
  case CANCEL:    return 280;
  case NWANDS:    error("Unknown type NWANDS");
  };
}

string& Wand::guess(Wand::Type subtype) {
  return guesses.at(static_cast<size_t>(subtype));
}

bool Wand::is_known(Wand::Type subtype) {
  return known.at(static_cast<size_t>(subtype));
}

void Wand::set_known(Wand::Type subtype) {
  known.at(static_cast<size_t>(subtype)) = true;
}

void Wand::init_wands() {
  static vector<string> possible_material = {
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

  while (materials.size() < static_cast<size_t>(Wand::NWANDS)) {
    string new_material = possible_material.at(os_rand_range(possible_material.size()));
    if (find(materials.cbegin(), materials.cend(), new_material) ==
        materials.cend()) {
      materials.push_back(new_material);
    }
  }
}

string const& Wand::material(Wand::Type subtype) {
  return materials.at(static_cast<size_t>(subtype));
}

void Wand::set_identified() {
  identified = true;
}

bool Wand::is_identified() const {
  return identified;
}


Wand::~Wand() {}

Wand::Wand() : Wand(random_wand_type()) {}

Wand::Wand(Wand::Type subtype_) : Item(), identified(false) {
  o_type = STICK;
  o_damage  = {1, 1};
  o_hurldmg = {1, 1};
  o_arm = 11;
  o_count = 1;
  o_which = subtype_;
  subtype = subtype_;

  if (subtype == LIGHT) {
    o_charges = os_rand_range(10) + 10;
  } else {
    o_charges = os_rand_range(5) + 3;
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

  if (is_known || !guess.empty()) {
    if (item_count(this) == 1) {
      os << "A wand";
    } else {
      os << to_string(item_count(this)) << " wands";
    }

    if (is_known) {
      os << " of " << Wand::name(subtype);
    } else if (!guess.empty()) {
      os << " called " << guess;
    }

    if (is_identified()) {
      os << " [" << item_charges(this) << " charges]";
    }

    os << " (" << Wand::material(subtype) << ")";

  } else {
    if (item_count(this) == 1) {
      os << "A " << Wand::material(subtype) << " wand";
    } else {
      os << to_string(item_count(this)) << " " << Wand::material(subtype) << " wands";
    }
  }

  return os.str();
}

string wand_description(Item const* item) {
  Wand const* wand = dynamic_cast<Wand const*>(item);
  if (wand == nullptr) {
    error("Cannot describe non-wand as wand");
  }
  return wand->get_description();
}
