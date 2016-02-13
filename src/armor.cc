#include <string>
#include <vector>
#include <sstream>

#include "game.h"
#include "disk.h"
#include "error_handling.h"
#include "io.h"
#include "item.h"
#include "misc.h"
#include "options.h"
#include "os.h"
#include "player.h"
#include "rings.h"
#include "rogue.h"

#include "armor.h"

using namespace std;

Armor::~Armor() {}

bool Armor::is_magic() const {
  return (rustproof ||
      get_armor() != Armor::ac(subtype));
}

class Armor* Armor::clone() const {
  return new Armor(*this);
}

static Armor::Type random_armor_type() {
  vector<Armor::Type> potential_armor;

  switch (Game::current_level) {
    default:

      [[clang::fallthrough]];
    case 50:
      potential_armor.push_back(Armor::Mithrilchainmail);

      [[clang::fallthrough]];
    case 40:
      potential_armor.push_back(Armor::Lamellararmor);

      [[clang::fallthrough]];
    case 30:
      potential_armor.push_back(Armor::Laminatedarmor);

      [[clang::fallthrough]];
    case 25:
      potential_armor.push_back(Armor::Brigandinearmor);

      [[clang::fallthrough]];
    case 20:
      potential_armor.push_back(Armor::Scalemail);

      [[clang::fallthrough]];
    case 15:
      potential_armor.push_back(Armor::Chainmail);

      [[clang::fallthrough]];
    case 12:
      potential_armor.push_back(Armor::Hardleatherringmail);

      [[clang::fallthrough]];
    case 10:
      potential_armor.push_back(Armor::Softleatherringmail);

      [[clang::fallthrough]];
    case 7:
      potential_armor.push_back(Armor::Hardstuddedleather);

      [[clang::fallthrough]];
    case 5:
      potential_armor.push_back(Armor::Hardleatherarmor);

      [[clang::fallthrough]];
    case 3:
      potential_armor.push_back(Armor::Softstuddedleather);

      [[clang::fallthrough]];
    case 2:
      potential_armor.push_back(Armor::Softleatherarmor);

      [[clang::fallthrough]];
    case 1:
      potential_armor.push_back(Armor::Robe);

  }

  return potential_armor.at(os_rand_range(potential_armor.size()));
}

Armor::Armor(bool random_stats) :
  Armor(random_armor_type(), random_stats)
{}

Armor::Armor(std::ifstream& data) {
  load(data);
}

Armor::Armor(Armor::Type type, bool random_stats) :
  Item(), subtype(type), identified(false), rustproof(false) {
  o_type = IO::Armor;
  o_which = type;
  set_armor(Armor::ac(type));

  if (random_stats) {
    int rand = os_rand_range(100);
    if (rand < 20) {
      set_cursed();
      modify_armor(os_rand_range(3) + 1);
    }
    else if (rand < 28) {
      modify_armor(-os_rand_range(3) + 1);
    }
  }
}

void Armor::set_identified() {
  identified = true;
}

bool Armor::is_identified() const {
  return identified;
}

string Armor::name(Armor::Type type) {
  switch (type) {
    case Robe:                  return "robe";
    case Softleatherarmor:      return "soft leather armor";
    case Softstuddedleather:    return "soft studded leather";
    case Hardleatherarmor:      return "hard leather armor";
    case Hardstuddedleather:    return "hard studded leather";
    case Softleatherringmail:   return "soft leather ringmail";
    case Hardleatherringmail:   return "hard leather ringmail";
    case Chainmail:             return "chainmail";
    case Scalemail:             return "scalemail";
    case Brigandinearmor:       return "brigandine armor";
    case Laminatedarmor:        return "laminated armor";
    case Lamellararmor:         return "lamellar armor";
    case Mithrilchainmail:      return "mithril chainmail";

    case NARMORS:         error("Unknown type NARMORS");
  }
}

int Armor::value(Armor::Type type) {
  switch (type) {
    case Robe:                  return 1;
    case Softleatherarmor:      return 5;
    case Softstuddedleather:    return 25;
    case Hardleatherarmor:      return 50;
    case Hardstuddedleather:    return 125;
    case Softleatherringmail:   return 170;
    case Hardleatherringmail:   return 250;
    case Chainmail:             return 350;
    case Scalemail:             return 530;
    case Brigandinearmor:       return 670;
    case Laminatedarmor:        return 800;
    case Lamellararmor:         return 1000;
    case Mithrilchainmail:      return 5000;

    case NARMORS:         error("Unknown type NARMORS");
  }
}

int Armor::ac(Armor::Type type) {
  switch (type) {
    case Robe:                  return 2;
    case Softleatherarmor:      return 4;
    case Softstuddedleather:    return 5;
    case Hardleatherarmor:      return 6;
    case Hardstuddedleather:    return 7;
    case Softleatherringmail:   return 8;
    case Hardleatherringmail:   return 10;
    case Chainmail:             return 12;
    case Scalemail:             return 13;
    case Brigandinearmor:       return 14;
    case Laminatedarmor:        return 16;
    case Lamellararmor:         return 20;
    case Mithrilchainmail:      return 30;

    case NARMORS:         error("Unknown type NARMORS");
  }
}

string Armor::get_description() const {
  stringstream buffer;

  string const& obj_name = Armor::name(static_cast<Armor::Type>(o_which));
  int bonus_ac = Armor::ac(static_cast<Armor::Type>(o_which)) - get_armor();
  int base_ac = 10 - get_armor() - bonus_ac;

  buffer
    << "a"
    << vowelstr(obj_name)
    << " "
    << obj_name
    << " ["
    << base_ac;

  if (identified) {
    buffer << ",";
    if (bonus_ac > 0) {
      buffer << "+";
    }
    buffer << bonus_ac;
  }

  buffer << "]";

  if (!get_nickname().empty()) {
    buffer << " {" << get_nickname() << "}";
  }

  return buffer.str();
}

void Armor::save(std::ofstream& data) const {
  Item::save(data);
  static_assert(sizeof(Armor::Type) == sizeof(int), "Wrong Armor::Type size");
  Disk::save(TAG_ARMOR, static_cast<int>(subtype), data);
  Disk::save(TAG_ARMOR, identified, data);
}

bool Armor::load(std::ifstream& data) {
  if (!Item::load(data) ||
      !Disk::load(TAG_ARMOR, reinterpret_cast<int&>(subtype), data) ||
      !Disk::load(TAG_ARMOR, identified, data)) {
    return false;
  }
  return true;
}


void Armor::set_rustproof() {
  rustproof = true;
}

bool Armor::is_rustproof() const {
  return rustproof;
}
