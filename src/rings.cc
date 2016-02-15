#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include "options.h"
#include "game.h"
#include "disk.h"
#include "error_handling.h"
#include "daemons.h"
#include "io.h"
#include "item.h"
#include "misc.h"
#include "options.h"
#include "os.h"
#include "player.h"
#include "rogue.h"
#include "weapons.h"
#include "monster.h"

#include "rings.h"

using namespace std;

vector<string>* Ring::materials = nullptr;
vector<string>* Ring::guesses = nullptr;
vector<bool>*   Ring::known = nullptr;

static Ring::Type random_ring_type() {
  vector<Ring::Type> potential_rings;

  switch (Game::current_level) {
    default:

      [[clang::fallthrough]];
    case 50:
      potential_rings.push_back(Ring::Speed);

      [[clang::fallthrough]];
    case 45: case 46: case 47: case 48: case 49:
    case 40: case 41: case 42: case 43: case 44:
      potential_rings.push_back(Ring::SeeInvisible);
      potential_rings.push_back(Ring::SustainStrenght);

      [[clang::fallthrough]];
    case 35: case 36: case 37: case 38: case 39:
    case 30: case 31: case 32: case 33: case 34:
      potential_rings.push_back(Ring::Strength);

      [[clang::fallthrough]];
    case 25: case 26: case 27: case 28: case 29:
    case 20: case 21: case 22: case 23: case 24:
      potential_rings.push_back(Ring::Damage);
      potential_rings.push_back(Ring::Accuracy);
      potential_rings.push_back(Ring::Regeneration);
      potential_rings.push_back(Ring::Stealth);

      [[clang::fallthrough]];
    case 15: case 16: case 17: case 18: case 19:
    case 10: case 11: case 12: case 13: case 14:
    case 7: case 8: case 9:
      potential_rings.push_back(Ring::AggravateMonsters);
      potential_rings.push_back(Ring::Teleportation);
      potential_rings.push_back(Ring::Protection);
      potential_rings.push_back(Ring::Searching);
      potential_rings.push_back(Ring::SlowDigestation);

      [[clang::fallthrough]];
    case 5: case 6:
    case 1: case 2: case 3: case 4:
      potential_rings.push_back(Ring::Adornment);
  }

  return potential_rings.at(os_rand_range(potential_rings.size()));
}



Ring::~Ring() {}

Ring::Ring() : Ring(random_ring_type()) {}

Ring::Ring(std::ifstream& data) {
  load(data);
}

Ring::Ring(Ring::Type type)
  : Item(), subtype(type), identified(false) {
  o_type = IO::Ring;
  o_which = type;

  switch (type) {

    // -20 - 20
    case Ring::Strength:
    case Ring::Protection: case Ring::Damage: case Ring::Accuracy: {
      switch (os_rand_range(100)) {
        case  0: set_armor(-20); break;
        case  1: set_armor(-19); break;
        case  2: set_armor(-18); break;
        case  3: case  4: set_armor(-17); break;
        case  5: case  6: set_armor(-16); break;
        case  7: case  8: set_armor(-15); break;
        case  9: case 10: set_armor(-14); break;
        case 11: case 12: set_armor(-13); break;
        case 13: case 14: set_armor(-12); break;
        case 15: case 16: set_armor(-11); break;
        case 17: case 18: set_armor(-10); break;
        case 19: case 20: set_armor(-9); break;
        case 21: case 22: set_armor(-7); break;
        case 23: case 24: case 25: set_armor(-6); break;
        case 26: case 27: case 28: set_armor(-5); break;
        case 29: case 30: case 31: set_armor(-4); break;
        case 32: case 33: case 34: set_armor(-3); break;
        case 35: case 36: case 37: case 38: set_armor(-2); break;
        case 39: case 40: case 41: case 42: case 43: case 44: set_armor(-1); break;
        case 45: case 46: case 47: case 48: case 49: case 50: case 51: set_armor(1); break;
        case 52: case 53: case 54: case 55: case 56: case 57: set_armor(2); break;
        case 58: case 59: case 60: case 61: set_armor(3); break;
        case 62: case 63: case 64: case 65: set_armor(4); break;
        case 66: case 67: case 68: case 69: set_armor(5); break;
        case 70: case 71: case 72: set_armor(6); break;
        case 73: case 74: case 75: set_armor(7); break;
        case 76: case 77: case 78: set_armor(8); break;
        case 79: case 80: case 81: set_armor(9); break;
        case 82: case 83: case 84: set_armor(10); break;
        case 85: case 86: set_armor(11); break;
        case 87: case 88: set_armor(12); break;
        case 89: case 90: set_armor(13); break;
        case 91: case 92: set_armor(15); break;
        case 93: case 94: set_armor(16); break;
        case 95: case 96: set_armor(17); break;
        case 97: set_armor(18); break;
        case 98: set_armor(19); break;
        case 99: set_armor(20); break;
      }

      if (get_armor() < 0) {
        set_cursed();
      }
    } break;

    // Cursed things
    case Ring::AggravateMonsters: case Ring::Teleportation: {
      set_cursed();
    } break;

    case Ring::Adornment: case Ring::Searching: case Ring::SlowDigestation:
    case Ring::Regeneration: case Ring::Stealth: case Ring::SeeInvisible:
    case Ring::SustainStrenght: case Ring::Speed: {
      break;
    }

    case Ring::NRINGS: error("Unknown type NRINGS");
  }
}

Ring* Ring::clone() const {
  return new Ring(*this);
}

bool Ring::is_magic() const {
  return true;
}

string Ring::name(Ring::Type type) {
  switch (type) {
    case Adornment:           return "adornment";
    case AggravateMonsters:   return "aggravate monster";
    case Teleportation:       return "teleportation";
    case Protection:          return "protection";
    case Searching:           return "searching";
    case SlowDigestation:     return "slow digestation";
    case Damage:              return "damage";
    case Accuracy:            return "accuracy";
    case Regeneration:        return "regeneration";
    case Stealth:             return "stealth";
    case Strength:            return "strength";
    case SeeInvisible:        return "see invisible";
    case SustainStrenght:     return "sustain strenght";
    case Speed:               return "speed";

    case NRINGS:            error("Unknown ring NRINGS");
  }
}

int Ring::worth(Ring::Type type) {
  switch (type) {
    case Adornment:           return 40;
    case AggravateMonsters:   return 0;
    case Teleportation:       return 0;
    case Protection:          return 100;
    case Searching:           return 250;
    case SlowDigestation:     return 200;
    case Damage:              return 100;
    case Accuracy:            return 100;
    case Regeneration:        return 400;
    case Stealth:             return 400;
    case Strength:            return 400;
    case SeeInvisible:        return 500;
    case SustainStrenght:     return 650;
    case Speed:               return 3000;

    case NRINGS:   error("Unknown ring NRINGS");
  }
}

string& Ring::guess(Ring::Type type) {
  return guesses->at(static_cast<size_t>(type));
}

bool Ring::is_known(Ring::Type type) {
  return known->at(static_cast<size_t>(type));
}

void Ring::set_known(Ring::Type type) {
  known->at(static_cast<size_t>(type)) = true;
}

void Ring::init_rings() {
  materials = new vector<string>;
  guesses = new vector<string>(Ring::NRINGS, "");
  known = new vector<bool>(Ring::NRINGS, false);

  vector<string> stones {
    "agate",     "alexandrite", "amethyst",       "carnelian", "diamond",    "emerald",
    "germanium", "granite",     "garnet",         "jade",      "kryptonite", "lapis lazuli",
    "moonstone", "obsidian",    "onyx",           "opal",      "pearl",      "peridot",
    "ruby",      "sapphire",    "stibotantalite", "tiger eye", "topaz",      "turquoise",
    "taaffeite", "zircon"
  };

  while (materials->size() < static_cast<size_t>(Ring::Type::NRINGS)) {
    size_t stone = static_cast<size_t>(os_rand_range(stones.size()));

    if (find(materials->begin(), materials->end(), stones.at(stone)) != materials->end())
      continue;

    materials->push_back(stones.at(stone));
  }

  // Run some checks
  if (materials->size() != static_cast<size_t>(Ring::NRINGS)) {
    error("Ring init: wrong number of materials");
  } else if (known->size() != static_cast<size_t>(Ring::NRINGS)) {
    error("Ring init: wrong number of knowledge");
  } else if (guesses->size() != static_cast<size_t>(Ring::NRINGS)) {
    error("Ring init: wrong number of guesses");
  }
}

void Ring::save_rings(std::ofstream& data) {
  Disk::save_tag(TAG_RINGS, data);
  Disk::save(TAG_MATERIALS, materials, data);
  Disk::save(TAG_KNOWN, known, data);
  Disk::save(TAG_GUESSES, guesses, data);
}

void Ring::load_rings(std::ifstream& data) {
  if (!Disk::load_tag(TAG_RINGS, data))             { error("No Rings found"); }
  if (!Disk::load(TAG_MATERIALS, materials, data)) { error("Ring tag error 1"); }
  if (!Disk::load(TAG_KNOWN, known, data))         { error("Ring tag error 2"); }
  if (!Disk::load(TAG_GUESSES, guesses, data))     { error("Ring tag error 3"); }
}

void Ring::free_rings() {
  delete materials;
  materials = nullptr;

  delete known;
  known = nullptr;

  delete guesses;
  guesses = nullptr;
}

std::string Ring::get_description() const {
  stringstream os;

  os
    << "a"
    << vowelstr(materials->at(subtype))
    << " "
    << materials->at(subtype)
    << " ring";

  if (Ring::is_known(subtype)) {
    os << " of " << Ring::name(subtype);

    switch (subtype) {
      case Ring::Protection: case Ring::Strength: case Ring::Damage:
      case Ring::Accuracy: {
        if (identified) {
          if (get_armor() > 0) {
            os << " [+" << get_armor() << "]";
          } else {
            os << " [" << get_armor() << "]";
          }
        } else {
          os << " [+/-?]";
        }
      } break;

      case Ring::Adornment: case Ring::AggravateMonsters: case Ring::Teleportation:
      case Ring::Searching: case Ring::SlowDigestation: case Ring::Regeneration:
      case Ring::Stealth: case Ring::SeeInvisible: case Ring::SustainStrenght:
      case Ring::Speed:
        break;

    case NRINGS:   error("Unknown ring NRINGS");
    }

  } else if (!Ring::guess(subtype).empty()) {
    os << " {" << Ring::guess(subtype) << "}";
  }

  return os.str();
}

void Ring::set_identified() {
  identified = true;
  set_known(subtype);
}

bool Ring::is_identified() const {
  return identified;
}
void Ring::save(std::ofstream& data) const {
  Item::save(data);
  static_assert(sizeof(Ring::Type) == sizeof(int), "Wrong Ring::Type size");
  Disk::save(TAG_RINGS, static_cast<int>(subtype), data);
  Disk::save(TAG_RINGS, identified, data);
}

bool Ring::load(std::ifstream& data) {
  if (!Item::load(data) ||
      !Disk::load(TAG_RINGS, reinterpret_cast<int&>(subtype), data) ||
      !Disk::load(TAG_RINGS, identified, data)) {
    return false;
  }
  return true;
}

int Ring::get_base_value() const {
  return worth(subtype);
}

int Ring::get_value() const {
  int value = get_base_value();
  value += 100 * get_armor();

  if (!is_identified()) {
    value /= 2;
  }
  value *= o_count;

  if (value < 0) {
    value = 0;
  }

  return value;
}

bool Ring::is_stackable() const {
  return true;
}
bool Ring::autopickup() const {
  return option_autopickup(o_type);
}
