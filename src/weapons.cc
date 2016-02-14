#include <vector>
#include <string>
#include <sstream>

#include "disk.h"
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
#include "player.h"
#include "rogue.h"

#include "weapons.h"

using namespace std;

static Weapon::Type random_weapon_type() {
  vector<Weapon::Type> potential_weapons;

  switch (Game::current_level) {
    default:

      [[clang::fallthrough]];
    case 30:
      potential_weapons.push_back(Weapon::Claymore);
      potential_weapons.push_back(Weapon::Nodachi);
      potential_weapons.push_back(Weapon::Warpike);
      potential_weapons.push_back(Weapon::Compositebow);

      [[clang::fallthrough]];
    case 25: case 26: case 27: case 28: case 29:
    case 20: case 21: case 22: case 23: case 24:
      potential_weapons.push_back(Weapon::Bastardsword);
      potential_weapons.push_back(Weapon::Halberd);
      potential_weapons.push_back(Weapon::Katana);

      [[clang::fallthrough]];
    case 15: case 16: case 17: case 18: case 19:
      potential_weapons.push_back(Weapon::Battleaxe);
      potential_weapons.push_back(Weapon::Warhammer);
      potential_weapons.push_back(Weapon::Yari);

      [[clang::fallthrough]];
    case 10: case 11: case 12: case 13: case 14:
      potential_weapons.push_back(Weapon::Morningstar);
      potential_weapons.push_back(Weapon::Longsword);
      potential_weapons.push_back(Weapon::Wakizashi);
      potential_weapons.push_back(Weapon::Longbow);
      potential_weapons.push_back(Weapon::Throwingaxe);

      [[clang::fallthrough]];
    case 5: case 6: case 7: case 8: case 9:
      potential_weapons.push_back(Weapon::Mace);
      potential_weapons.push_back(Weapon::Spear);
      potential_weapons.push_back(Weapon::Handaxe);
      potential_weapons.push_back(Weapon::Kukri);
      potential_weapons.push_back(Weapon::Shortbow);
      potential_weapons.push_back(Weapon::Throwingknife);

      [[clang::fallthrough]];
    case 3: case 4:
      potential_weapons.push_back(Weapon::Shortsword);
      potential_weapons.push_back(Weapon::Rapier);

      [[clang::fallthrough]];
    case 1: case 2:
      potential_weapons.push_back(Weapon::Sling);
      potential_weapons.push_back(Weapon::Arrow);
      potential_weapons.push_back(Weapon::Rock);
      potential_weapons.push_back(Weapon::Dagger);
      potential_weapons.push_back(Weapon::Club);
      potential_weapons.push_back(Weapon::Quarterstaff);
  }

  return potential_weapons.at(os_rand_range(potential_weapons.size()));
}

class Weapon* Weapon::clone() const {
  return new Weapon(*this);
}

bool Weapon::is_magic() const {
  return get_hit_plus() != 0 || get_damage_plus() != 0;
}

Weapon::~Weapon() {}

Weapon::Weapon(bool random_stats) : Weapon(random_weapon_type(), random_stats) {}

Weapon::Weapon(std::ifstream& data) {
  load(data);
}

Weapon::Weapon(Weapon::Type subtype_, bool random_stats)
  : Item(), subtype(subtype_), is_ammo_type(AmmoType::None),
    uses_ammo_type(AmmoType::None), ammo_multiplier(0),
    identified(false), good_missile(false)  {

  o_type = IO::Weapon;
  o_which = subtype;
  o_count = 1;

  switch (subtype) {
    case Weapon::Sling: {
      set_attack_damage({0,0});
      set_throw_damage({1,1});
      ammo_multiplier = 2;
      uses_ammo_type = AmmoType::SlingShot;
    } break;

    case Weapon::Arrow: {
      set_attack_damage({1,1});
      set_throw_damage({1,4});
      is_ammo_type = AmmoType::BowArrow;
      o_count = os_rand_range(8) + 8;
      good_missile = true;
      o_type = IO::Ammo;
    } break;

    case Weapon::Rock: {
      set_attack_damage({1,1});
      set_throw_damage({1,2});
      is_ammo_type = AmmoType::SlingShot;
      o_count = os_rand_range(8);
      good_missile = true;
      o_type = IO::Ammo;
    } break;

    case Dagger: {
      set_attack_damage({1,4});
      set_throw_damage({1,4});
      good_missile = true;
    } break;

    case Club: {
      set_attack_damage({1,3});
      set_throw_damage({1,1});
    } break;

    case Quarterstaff: {
      set_attack_damage({1,4});
      set_throw_damage({1,1});
      set_armor(1);
    } break;

    case Shortsword: {
      set_attack_damage({1,6});
      set_throw_damage({1,2});
    } break;

    case Rapier: {
      set_attack_damage({1,6});
      set_throw_damage({1,2});
    } break;

    case Mace: {
      set_attack_damage({2,4});
      set_throw_damage({1,3});
    } break;

    case Spear: {
      set_attack_damage({1,6});
      set_throw_damage({1,12});
      set_armor(2);
      good_missile = true;
    } break;

   case Handaxe: {
      set_attack_damage({1,6});
      set_throw_damage({1,6});
    } break;

    case Kukri: {
      set_attack_damage({1,6});
      set_throw_damage({1,4});
    } break;

    case Shortbow: {
      set_attack_damage({1,2});
      set_throw_damage({0,0});
      ammo_multiplier = 2;
      uses_ammo_type = BowArrow;
    } break;

    case Throwingknife: {
      set_attack_damage({1,4});
      set_throw_damage({2,3});
      good_missile = true;
      o_count = os_rand_range(4) + 2;
    } break;

    case Morningstar: {
      set_attack_damage({2,6});
      set_throw_damage({1,2});
    } break;

    case Longsword: {
      set_attack_damage({1,10});
      set_throw_damage({1,2});
    } break;

    case Wakizashi: {
      set_attack_damage({2,4});
      set_throw_damage({1,4});
    } break;

    case Longbow: {
      set_attack_damage({1,2});
      set_throw_damage({0,0});
      ammo_multiplier = 3;
      uses_ammo_type = BowArrow;
    } break;

    case Throwingaxe: {
      set_attack_damage({1,6});
      set_throw_damage({2,4});
      good_missile = true;
      o_count = os_rand_range(4) + 2;
    } break;

    case Battleaxe: {
      set_attack_damage({3,4});
      set_throw_damage({1,4});
    } break;

    case Warhammer: {
      set_attack_damage({3,3});
      set_throw_damage({1,2});
    } break;

    case Yari: {
      set_attack_damage({1,10});
      set_throw_damage({1,12});
      set_armor(3);
      good_missile = true;
    } break;

    case Katana: {
      set_attack_damage({1,12});
      set_throw_damage({1,2});
    } break;

    case Halberd: {
      set_attack_damage({2,4});
      set_throw_damage({1,3});
      set_armor(4);
    } break;

    case Bastardsword: {
      set_attack_damage({3,4});
      set_throw_damage({1,3});
    } break;

    case Compositebow: {
      set_attack_damage({1,2});
      set_throw_damage({0,0});
      ammo_multiplier = 4;
      uses_ammo_type = BowArrow;
    } break;

    case Warpike: {
      set_attack_damage({1,12});
      set_throw_damage({1,12});
      set_armor(3);
    } break;

    case Nodachi: {
      set_attack_damage({3,6});
      set_throw_damage({1,2});
    } break;

    case Claymore: {
      set_attack_damage({4,4});
      set_throw_damage({1,2});
    } break;

    case NWEAPONS: error("Unknown type NWEAPONS");
    case NO_WEAPON: error("Unknown type NO_WEAPON");
  }

  if (random_stats) {
    int rand = os_rand_range(100);
    if (rand < 10) {
      set_cursed();
      modify_hit_plus(-os_rand_range(3) + 1);
    }
    else if (rand < 15) {
      modify_hit_plus(os_rand_range(3) + 1);
    }
  }
}

void Weapon::set_identified() {
  identified = true;
}

bool Weapon::is_identified() const {
  return identified;
}

string Weapon::name(Weapon::Type type) {
  switch (type) {
    case Sling: return "sling";
    case Arrow: return "arrow";
    case Rock: return "rock";
    case Dagger: return "dagger";
    case Club: return "club";
    case Quarterstaff: return "quarterstaff";
    case Shortbow: return "shortbow";
    case Throwingknife: return "throwing knife";
    case Mace: return "cace";
    case Spear: return "spear";
    case Rapier: return "rapier";
    case Kukri: return "kukri";
    case Handaxe: return "hand axe";
    case Shortsword: return "shortsword";
    case Longsword: return "longsword";
    case Wakizashi: return "wakizashi";
    case Longbow: return "longbow";
    case Throwingaxe: return "throwing axe";
    case Morningstar: return "morningstar";
    case Battleaxe: return "battle axe";
    case Warhammer: return "war hammer";
    case Yari: return "yari";
    case Bastardsword: return "bastard sword";
    case Halberd: return "halberd";
    case Katana: return "katana";
    case Claymore: return "claymore";
    case Nodachi: return "nodachi";
    case Warpike: return "warpike";
    case Compositebow: return "compositebow";

    case NWEAPONS: error("Unknown type NWEAPONS");
    case NO_WEAPON: error("Unknown type NO_WEAPON");
  }
}

int Weapon::worth(Weapon::Type type) {
  switch (type) {
    case Sling:         return 5;
    case Arrow:         return 5;
    case Rock:          return 2;
    case Dagger:        return 10;
    case Club:          return 2;
    case Quarterstaff:  return 3;
    case Shortbow:      return 50;
    case Throwingknife: return 20;
    case Mace:          return 240;
    case Spear:         return 70;
    case Rapier:        return 40;
    case Kukri:         return 70;
    case Handaxe:       return 30;
    case Shortsword:    return 40;
    case Longsword:     return 200;
    case Wakizashi:     return 230;
    case Longbow:       return 120;
    case Throwingaxe:   return 40;
    case Morningstar:   return 360;
    case Battleaxe:     return 340;
    case Warhammer:     return 320;
    case Yari:          return 540;
    case Bastardsword:  return 350;
    case Halberd:       return 600;
    case Katana:        return 360;
    case Claymore:      return 800;
    case Nodachi:       return 800;
    case Warpike:       return 650;
    case Compositebow:  return 350;

    case NWEAPONS: error("Unknown type NWEAPONS");
    case NO_WEAPON: error("Unknown type NO_WEAPON");
  }
}

string Weapon::get_description() const {
  stringstream buffer;

  string const& obj_name = Weapon::name(static_cast<Weapon::Type>(o_which));

  if (o_count == 1) {
    buffer << "a" << vowelstr(obj_name) << " " << obj_name;
  } else {
    buffer << o_count << " " << obj_name << "s";
  }

  if (uses_ammo_type != AmmoType::None) {
    buffer << " (x" << ammo_multiplier << ")";
  } else if (o_type == IO::Ammo) {
    buffer
      << " (" << get_throw_damage().dices
      << "d"  << get_throw_damage().sides << ")";
  } else {
    buffer
      << " (" << get_attack_damage().dices
      << "d"  << get_attack_damage().sides << ")";
  }

  if (identified) {
    buffer << " (";
    int p_hit = get_hit_plus();
    if (p_hit >= 0) {
      buffer << "+";
    }
    buffer << p_hit << ",";

    int p_dmg = get_damage_plus();
    if (p_dmg >= 0) {
      buffer << "+";
    }
    buffer << p_dmg << ")";
  }

  if (get_armor() != 0) {
    buffer << " [";
    int p_armor = get_armor();
    if (p_armor >= 0) {
      buffer << "+";
    }
    buffer << p_armor << "]";
  }

  if (!get_nickname().empty()) {
    buffer << " {" << get_nickname() << "}";
  }

  return buffer.str();
}

void
weapon_missile_fall(Item* obj, bool pr) {
  Coordinate fpos;

  if (fallpos(&obj->get_position(), &fpos)) {
    obj->set_position(fpos);

    // See if we can stack it with something on the ground
    for (Item* ground_item : Game::level->items) {
      if (ground_item->get_position() == obj->get_position() &&
          ground_item->o_type == obj->o_type &&
          ground_item->o_which == obj->o_which &&
          ground_item->get_hit_plus() == obj->get_hit_plus() &&
          ground_item->get_damage_plus() == obj->get_damage_plus()) {

        ground_item->o_count++;
        delete obj;
        obj = nullptr;
        break;
      }
    }

    if (obj != nullptr) {
      Game::level->items.push_back(obj);
    }
    return;
  }

  if (pr) {
    stringstream os;
    os << "the "
       << Weapon::name(static_cast<Weapon::Type>(obj->o_which))
       << " vanishes as it hits the ground";
    Game::io->message(os.str());
  }
  delete obj;
}

void Weapon::save(std::ofstream& data) const {
  Item::save(data);
  static_assert(sizeof(Weapon::Type) == sizeof(int), "Wrong Weapon::Type size");
  static_assert(sizeof(Weapon::AmmoType) == sizeof(int), "Wrong AmmoType size");
  Disk::save(TAG_WEAPON, static_cast<int>(subtype), data);
  Disk::save(TAG_WEAPON, static_cast<int>(is_ammo_type), data);
  Disk::save(TAG_WEAPON, static_cast<int>(uses_ammo_type), data);
  Disk::save(TAG_WEAPON, ammo_multiplier, data);
  Disk::save(TAG_WEAPON, identified, data);
  Disk::save(TAG_WEAPON, good_missile, data);
}

bool Weapon::load(std::ifstream& data) {
  if (!Item::load(data) ||
      !Disk::load(TAG_WEAPON, reinterpret_cast<int&>(subtype), data) ||
      !Disk::load(TAG_WEAPON, reinterpret_cast<int&>(is_ammo_type), data) ||
      !Disk::load(TAG_WEAPON, reinterpret_cast<int&>(uses_ammo_type), data) ||
      !Disk::load(TAG_WEAPON, ammo_multiplier, data) ||
      !Disk::load(TAG_WEAPON, identified, data) ||
      !Disk::load(TAG_WEAPON, good_missile, data)) {
    return false;
  }
  return true;
}


bool Weapon::is_missile_launcher() const {
  return uses_ammo_type != AmmoType::None;
}

Weapon::AmmoType Weapon::get_ammo_used() const {
  return uses_ammo_type;
}

int Weapon::get_ammo_multiplier() const {
  return ammo_multiplier;
}

Weapon::AmmoType Weapon::get_ammo_type() const {
  return is_ammo_type;
}

int Weapon::get_base_value() const {
  return worth(subtype);
}

int Weapon::get_value() const {
  int value = get_base_value();
  value += 100 * (get_hit_plus() + get_damage_plus());
  value *= o_count;
  return value;
}
