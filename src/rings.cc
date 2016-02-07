#include <vector>
#include <string>
#include <sstream>

#include "error_handling.h"
#include "daemons.h"
#include "io.h"
#include "item.h"
#include "misc.h"
#include "options.h"
#include "os.h"
#include "pack.h"
#include "player.h"
#include "rogue.h"
#include "things.h"
#include "weapons.h"
#include "monster.h"

#include "rings.h"

using namespace std;

vector<string>* Ring::materials = nullptr;
vector<string>* Ring::guesses = nullptr;
vector<bool>*   Ring::known = nullptr;

static Ring::Type random_ring_type() {
  int value = os_rand_range(100);

  int end = static_cast<int>(Ring::Type::NRINGS);
  for (int i = 0; i < end; ++i) {
    Ring::Type type = static_cast<Ring::Type>(i);
    int probability = Ring::probability(type);

    if (value < probability) {
      return type;

    } else {
      value -= probability;
    }
  }

  error("Error! Sum of probabilities is not 100%");
}



Ring::~Ring() {}

Ring::Ring(bool random_stats) : Ring(random_ring_type(), random_stats) {}

Ring::Ring(Ring::Type type, bool random_stats) : Item(), subtype(type) {
  o_type = RING;
  o_which = type;

  switch (o_which) {
    case ADDSTR: case PROTECT: case ADDHIT: case ADDDAM: {
      if (random_stats) {
        set_armor(os_rand_range(3));
        if (get_armor() == 0) {
          set_armor(-1);
          set_cursed();
        }

      } else {
        set_armor(1);
      }
    } break;

    case AGGR: case TELEPORT: {
      set_cursed();
    } break;
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
    case PROTECT:  return "protection";
    case ADDSTR:   return "add strength";
    case SUSTSTR:  return "sustain strength";
    case SEARCH:   return "searching";
    case SEEINVIS: return "see invisible";
    case NOP:      return "adornment";
    case AGGR:     return "aggravate monster";
    case ADDHIT:   return "dexterity";
    case ADDDAM:   return "increase damage";
    case REGEN:    return "regeneration";
    case DIGEST:   return "slow digestion";
    case TELEPORT: return "teleportation";
    case STEALTH:  return "stealth";
    case SUSTARM:  return "maintain armor";
    case NRINGS:   error("Unknown ring NRINGS");
  }
}

int Ring::probability(Ring::Type type) {
  switch (type) {
    case PROTECT:  return  9;
    case ADDSTR:   return  9;
    case SUSTSTR:  return  5;
    case SEARCH:   return 10;
    case SEEINVIS: return 10;
    case NOP:      return  1;
    case AGGR:     return 10;
    case ADDHIT:   return  8;
    case ADDDAM:   return  8;
    case REGEN:    return  4;
    case DIGEST:   return  9;
    case TELEPORT: return  5;
    case STEALTH:  return  7;
    case SUSTARM:  return  5;
    case NRINGS:   error("Unknown ring NRINGS");
  }
}

int Ring::worth(Ring::Type type) {
  switch (type) {
    case PROTECT:  return 400;
    case ADDSTR:   return 400;
    case SUSTSTR:  return 280;
    case SEARCH:   return 420;
    case SEEINVIS: return 310;
    case NOP:      return  10;
    case AGGR:     return  10;
    case ADDHIT:   return 440;
    case ADDDAM:   return 400;
    case REGEN:    return 460;
    case DIGEST:   return 240;
    case TELEPORT: return  30;
    case STEALTH:  return 470;
    case SUSTARM:  return 380;
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

  os << materials->at(subtype) << " ring";

  if (Ring::is_known(subtype)) {
    os << " of " << Ring::name(subtype);

    switch (subtype) {
      case Ring::PROTECT: case Ring::ADDSTR: case Ring::ADDDAM: case Ring::ADDHIT: {
        if (get_armor() > 0) {
          os << " [+" << get_armor() << "]";
        } else {
          os << " [" << get_armor() << "]";
        }
      } break;

    case SUSTSTR: case SEARCH: case SEEINVIS: case NOP: case AGGR:
    case REGEN: case DIGEST: case TELEPORT: case STEALTH: case SUSTARM:
        break;

    case NRINGS:   error("Unknown ring NRINGS");
    }

  } else if (!Ring::guess(subtype).empty()) {
    os << " called " << Ring::guess(subtype);
  }

  return os.str();
}

