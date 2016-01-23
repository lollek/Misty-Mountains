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

vector<string> Ring::materials;
vector<string> Ring::guesses;
vector<bool>   Ring::known;

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
        o_arm = os_rand_range(3);
        if (o_arm == 0) {
          o_arm = -1;
          o_flags |= ISCURSED;
        }

      } else {
        o_arm = 1;
      }
    } break;

    case AGGR: case TELEPORT: {
      o_flags |= ISCURSED;
    } break;
  }
}

Ring* Ring::clone() const {
  return new Ring(*this);
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
  return guesses.at(static_cast<size_t>(type));
}

bool Ring::is_known(Ring::Type type) {
  return known.at(static_cast<size_t>(type));
}

void Ring::set_known(Ring::Type type) {
  known.at(static_cast<size_t>(type)) = true;
}

void Ring::init_rings() {
  vector<string> stones {
    "agate",     "alexandrite", "amethyst",       "carnelian", "diamond",    "emerald",
    "germanium", "granite",     "garnet",         "jade",      "kryptonite", "lapis lazuli",
    "moonstone", "obsidian",    "onyx",           "opal",      "pearl",      "peridot",
    "ruby",      "sapphire",    "stibotantalite", "tiger eye", "topaz",      "turquoise",
    "taaffeite", "zircon"
  };

  while (materials.size() < static_cast<size_t>(Ring::Type::NRINGS)) {
    size_t stone = static_cast<size_t>(os_rand_range(stones.size()));

    if (find(materials.begin(), materials.end(), stones.at(stone)) != end(materials))
      continue;

    materials.push_back(stones.at(stone));
    break;
  }
}

std::string Ring::get_description() const {
  stringstream os;

  os << materials.at(subtype) << " ring";

  if (Ring::is_known(subtype)) {
    os << " of " << Ring::name(subtype);

    switch (subtype) {
      case Ring::PROTECT: case Ring::ADDSTR: case Ring::ADDDAM: case Ring::ADDHIT:
        if (item_armor(this) > 0) {
          os << " [+" << item_armor(this) << "]";
        } else {
          os << " [" << item_armor(this) << "]";
        }

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

bool
ring_put_on(void)
{
  Item* obj = pack_get_item("put on", RING);

  /* Make certain that it is somethings that we want to wear */
  if (obj == nullptr)
    return false;

  if (obj->o_type != RING)
  {
    io_msg("not a ring");
    return ring_put_on();
  }

  /* Try to put it on */
  if (!pack_equip_item(obj))
  {
    io_msg("you already have a ring on each hand");
    return false;
  }
  pack_remove(obj, false, true);

  /* Calculate the effect it has on the poor guy. */
  switch (obj->o_which)
  {
    case Ring::AGGR: monster_aggravate_all(); break;
  }

  string msg = ring_description(obj);
  msg.at(0) = static_cast<char>(tolower(msg.at(0)));
  io_msg("now wearing %s", msg.c_str());
  return true;
}

bool
ring_take_off(void)
{
  enum equipment_pos ring;

  /* Try right, then left */
  if (pack_equipped_item(EQUIPMENT_RRING) != nullptr)
    ring = EQUIPMENT_RRING;
  else
    ring = EQUIPMENT_LRING;

  Item* obj = pack_equipped_item(ring);

  if (!pack_unequip(ring, false))
    return false;

  switch (obj->o_which)
  {
    case Ring::ADDSTR:
      break;

    case Ring::SEEINVIS:
      daemon_extinguish_fuse(daemon_function::remove_true_sight);
      break;
  }
  return true;
}

int
ring_drain_amount(void)
{
  int total_eat = 0;
  int uses[] = {
    1, /* R_PROTECT */  1, /* R_ADDSTR   */  1, /* R_SUSTSTR  */
    1, /* R_SEARCH  */  1, /* R_SEEINVIS */  0, /* R_NOP      */
    0, /* R_AGGR    */  1, /* R_ADDHIT   */  1, /* R_ADDDAM   */
    2, /* R_REGEN   */ -1, /* R_DIGEST   */  0, /* R_TELEPORT */
    1, /* R_STEALTH */  1, /* R_SUSTARM  */
  };

  for (int i = 0; i < PACK_RING_SLOTS; ++i)
  {
    Item *ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring != nullptr)
      total_eat += uses[ring->o_which];
  }

  return total_eat;
}

string ring_description(Item const* item) {
  Ring const* ring = dynamic_cast<Ring const*>(item);
  if (ring == nullptr) {
    error("Cannot describe non-ring as ring");
  }
  return ring->get_description();
}
