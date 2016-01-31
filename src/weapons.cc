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

Weapon::~Weapon() {}

Weapon::Weapon(bool random_stats) : Weapon(random_weapon_type(), random_stats) {}

Weapon::Weapon(Weapon::Type subtype_, bool random_stats)
  : Item(), subtype(subtype_)  {

  o_type = WEAPON;
  o_which = subtype;
  o_count = 1;

  switch (subtype) {
    case MACE: {
      o_damage = {2,4};
      o_hurldmg = {1,3};
      o_launch = NO_WEAPON;
    } break;

    case SWORD: {
      o_damage = {3,4};
      o_hurldmg = {1,2};
      o_launch = NO_WEAPON;
    } break;

    case BOW: {
      o_damage = {1,1};
      o_hurldmg = {2,3};
      o_launch = NO_WEAPON;
    } break;

    case ARROW: {
      o_damage = {0,0};
      o_hurldmg = {1,1};
      o_launch = BOW;
      o_count = os_rand_range(8) + 8;
      o_flags = ISMANY|ISMISL;
      o_type = AMMO;
    } break;

    case DAGGER: {
      o_damage = {1,6};
      o_hurldmg = {1,4};
      o_launch = NO_WEAPON;
      o_count = os_rand_range(4) + 2;
      o_flags = ISMISL;
    } break;

    case TWOSWORD: {
      o_damage = {4,4};
      o_hurldmg = {1,3};
      o_launch = NO_WEAPON;
    } break;

    case DART: {
      o_damage = {0,0};
      o_hurldmg = {1,3};
      o_launch = NO_WEAPON;
      o_count = os_rand_range(8) + 8;
      o_flags = ISMANY|ISMISL;
      o_type = AMMO;
    } break;

    case SHIRAKEN: {
      o_damage = {0,0};
      o_hurldmg = {2,4};
      o_launch = NO_WEAPON;
      o_count = os_rand_range(8) + 8;
      o_flags = ISMANY|ISMISL;
      o_type = AMMO;
    } break;

    case SPEAR: {
      o_damage = {2,3};
      o_hurldmg = {1,6};
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

  string const& obj_name = Weapon::name(static_cast<Weapon::Type>(item_subtype(this)));

  if (item_count(this) == 1) {
    buffer << "A" << vowelstr(obj_name) << " " << obj_name;
  } else {
    buffer << item_count(this) << " " << obj_name << "s";
  }

  int dices;
  int sides;
  if (item_type(this) == AMMO || item_subtype(this) == Weapon::BOW) {
    dices = item_throw_damage(this)->dices;
    sides = item_throw_damage(this)->sides;
  } else if (item_type(this) == WEAPON) {
    dices = item_damage(this)->dices;
    sides = item_damage(this)->sides;
  } else {
    error("Bad item type");
  }

  buffer << " (" << sides << "d" << dices << ")";

  if (item_is_known(this)) {
    buffer << " (";
    int p_hit = item_bonus_hit(this);
    if (p_hit >= 0) {
      buffer << "+";
    }
    buffer << p_hit << ",";

    int p_dmg = item_bonus_damage(this);
    if (p_dmg >= 0) {
      buffer << "+";
    }
    buffer << p_dmg << ")";
  }

  if (item_armor(this) != 0) {
    buffer << " [";
    int p_armor = item_armor(this);
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

  if (fallpos(&obj->get_pos(), &fpos)) {
    obj->set_pos(fpos);
    Game::level->items.push_back(obj);
    return;
  }

  if (pr) {
    io_msg("the %s vanishes as it hits the ground",
        Weapon::name(static_cast<Weapon::Type>(obj->o_which)).c_str());
  }
  delete obj;
}

