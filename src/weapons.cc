#include <vector>
#include <string>
#include <sstream>

#include "error_handling.h"
#include "game.h"
#include "coordinate.h"
#include "fight.h"
#include "io.h"
#include "item.h"
#include "level.h"
#include "misc.h"
#include "monster.h"
#include "options.h"
#include "os.h"
#include "pack.h"
#include "player.h"
#include "rogue.h"
#include "things.h"

#include "weapons.h"

using namespace std;

static Weapon::Type random_weapon_type() {
  int value = os_rand_range(100);

  int end = static_cast<int>(Weapon::Type::NWEAPONS);
  for (int i = 0; i < end; ++i) {
    Weapon::Type type = static_cast<Weapon::Type>(i);
    int probability = Weapon::probability(type);

    if (value < probability) {
      return type;

    } else {
      value -= probability;
    }
  }

  error("Error! Sum of probabilities is not 100%");
}

Weapon* Weapon::clone() const {
  return new Weapon(*this);
}

bool Weapon::is_magic() const {
  return o_hplus != 0 || o_dplus != 0;
}

Weapon::~Weapon() {}

Weapon::Weapon(bool random_stats) : Weapon(random_weapon_type(), random_stats) {}

Weapon::Weapon(Weapon::Type subtype_, bool random_stats)
  : Item(), subtype(subtype_)  {

  o_type = WEAPON;
  o_which = subtype;
  o_count = 1;

  switch (subtype) {
    case MACE: {
      set_attack_damage({2,4});
      set_throw_damage({1,3});
      o_launch = NO_WEAPON;
    } break;

    case SWORD: {
      set_attack_damage({3,4});
      set_throw_damage({1,2});
      o_launch = NO_WEAPON;
    } break;

    case BOW: {
      set_attack_damage({1,1});
      set_throw_damage({2,3});
      o_launch = NO_WEAPON;
    } break;

    case ARROW: {
      set_attack_damage({0,0});
      set_throw_damage({1,1});
      o_launch = BOW;
      o_count = os_rand_range(8) + 8;
      o_flags = ISMANY|ISMISL;
      o_type = AMMO;
    } break;

    case DAGGER: {
      set_attack_damage({1,6});
      set_throw_damage({1,4});
      o_launch = NO_WEAPON;
      o_count = os_rand_range(4) + 2;
      o_flags = ISMISL;
    } break;

    case TWOSWORD: {
      set_attack_damage({4,4});
      set_throw_damage({1,3});
      o_launch = NO_WEAPON;
    } break;

    case DART: {
      set_attack_damage({0,0});
      set_throw_damage({1,3});
      o_launch = NO_WEAPON;
      o_count = os_rand_range(8) + 8;
      o_flags = ISMANY|ISMISL;
      o_type = AMMO;
    } break;

    case SHIRAKEN: {
      set_attack_damage({0,0});
      set_throw_damage({2,4});
      o_launch = NO_WEAPON;
      o_count = os_rand_range(8) + 8;
      o_flags = ISMANY|ISMISL;
      o_type = AMMO;
    } break;

    case SPEAR: {
      set_attack_damage({2,3});
      set_throw_damage({1,6});
      o_arm = 2;
      o_launch = NO_WEAPON;
      o_flags = ISMISL;
    } break;

    case NWEAPONS: error("Unknown type NWEAPONS");
    case NO_WEAPON: error("Unknown type NO_WEAPON");
  }

  if (random_stats) {
    int rand = os_rand_range(100);
    if (rand < 10) {
      o_flags |= ISCURSED;
      o_hplus -= os_rand_range(3) + 1;
    }
    else if (rand < 15) {
      o_hplus += os_rand_range(3) + 1;
    }
  }
}

void Weapon::set_identified() {
  identified = true;
}

void Weapon::set_not_identified() {
  identified = false;
}

bool Weapon::is_identified() const {
  return identified;
}

string Weapon::name(Weapon::Type type) {
  switch (type) {
    case MACE:     return "mace";
    case SWORD:    return "long sword";
    case BOW:      return "short bow";
    case ARROW:    return "arrow";
    case DAGGER:   return "dagger";
    case TWOSWORD: return "two handed sword";
    case DART:     return "dart";
    case SHIRAKEN: return "shuriken";
    case SPEAR:    return "spear";
    case NWEAPONS: error("Unknown type NWEAPONS");
    case NO_WEAPON: error("Unknown type NO_WEAPON");
  }
}

int Weapon::probability(Weapon::Type type) {
  switch (type) {
    case MACE:     return 11;
    case SWORD:    return 11;
    case BOW:      return 12;
    case ARROW:    return 12;
    case DAGGER:   return  8;
    case TWOSWORD: return 10;
    case DART:     return 12;
    case SHIRAKEN: return 12;
    case SPEAR:    return 12;
    case NWEAPONS: error("Unknown type NWEAPONS");
    case NO_WEAPON: error("Unknown type NO_WEAPON");
  }
}

int Weapon::worth(Weapon::Type type) {
  switch (type) {
    case MACE:     return  8;
    case SWORD:    return 15;
    case BOW:      return 15;
    case ARROW:    return  1;
    case DAGGER:   return  3;
    case TWOSWORD: return 75;
    case DART:     return  2;
    case SHIRAKEN: return  5;
    case SPEAR:    return  5;
    case NWEAPONS: error("Unknown type NWEAPONS");
    case NO_WEAPON: error("Unknown type NO_WEAPON");
  }
}

string Weapon::get_description() const {
  stringstream buffer;

  string const& obj_name = Weapon::name(static_cast<Weapon::Type>(o_which));

  if (o_count == 1) {
    buffer << "A" << vowelstr(obj_name) << " " << obj_name;
  } else {
    buffer << o_count << " " << obj_name << "s";
  }

  int dices;
  int sides;
  if (o_type == AMMO || o_which == Weapon::BOW) {
    dices = get_throw_damage().dices;
    sides = get_throw_damage().sides;
  } else if (o_type == WEAPON) {
    dices = get_attack_damage().dices;
    sides = get_attack_damage().sides;
  } else {
    error("Bad item type");
  }

  buffer << " (" << sides << "d" << dices << ")";

  if (identified) {
    buffer << " (";
    int p_hit = o_hplus;
    if (p_hit >= 0) {
      buffer << "+";
    }
    buffer << p_hit << ",";

    int p_dmg = o_dplus;
    if (p_dmg >= 0) {
      buffer << "+";
    }
    buffer << p_dmg << ")";
  }

  if (o_arm != 0) {
    buffer << " [";
    int p_armor = o_arm;
    if (p_armor >= 0) {
      buffer << "+";
    }
    buffer << p_armor << "]";
  }

  if (!get_nickname().empty()) {
    buffer << " called " << get_nickname();
  }

  return buffer.str();
}

string weapon_description(Item const* item) {
  Weapon const* weapon = dynamic_cast<Weapon const*>(item);
  if (weapon == nullptr) {
    error("Cannot describe non-weapon as weapon");
  }
  return weapon->get_description();

}

void
weapon_missile_fall(Item* obj, bool pr) {
  Coordinate fpos;

  if (fallpos(&obj->get_position(), &fpos)) {
    obj->set_position(fpos);
    Game::level->items.push_back(obj);
    return;
  }

  if (pr) {
    io_msg("the %s vanishes as it hits the ground",
        Weapon::name(static_cast<Weapon::Type>(obj->o_which)).c_str());
  }
  delete obj;
}

