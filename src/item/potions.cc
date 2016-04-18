#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "colors.h"
#include "disk.h"
#include "error_handling.h"
#include "game.h"
#include "io.h"
#include "item.h"
#include "level.h"
#include "misc.h"
#include "monster.h"
#include "options.h"
#include "os.h"
#include "player.h"
#include "rogue.h"

#include "item/potions.h"

using namespace std;

vector<string>* Potion::guesses;
vector<bool>* Potion::knowledge;
vector<string>* Potion::colors;

static Potion::Type random_potion_type() {
  vector<Potion::Type> potential_potions;

  switch (Game::current_level) {
    default: [[clang::fallthrough]];
    case 15:
      potential_potions.push_back(Potion::XHEAL);
      potential_potions.push_back(Potion::RAISE);

      [[clang::fallthrough]];
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
      potential_potions.push_back(Potion::STRENGTH);
      potential_potions.push_back(Potion::MFIND);
      potential_potions.push_back(Potion::TFIND);

      [[clang::fallthrough]];
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      potential_potions.push_back(Potion::HEALING);
      potential_potions.push_back(Potion::RESTORE);

      [[clang::fallthrough]];
    case 3:
    case 4:
      potential_potions.push_back(Potion::POISON);
      potential_potions.push_back(Potion::SEEINVIS);

      [[clang::fallthrough]];
    case 1:
    case 2:
      potential_potions.push_back(Potion::CONFUSION);
      potential_potions.push_back(Potion::BLIND);
      potential_potions.push_back(Potion::LEVIT);
      potential_potions.push_back(Potion::HASTE);
  }

  return potential_potions.at(os_rand_range(potential_potions.size()));
}

Potion* Potion::clone() const { return new Potion(*this); }

bool Potion::is_magic() const { return true; }

string Potion::name(Potion::Type subtype) {
  switch (subtype) {
    case CONFUSION: return "confusion";
    case POISON: return "poison";
    case STRENGTH: return "gain strength";
    case SEEINVIS: return "see invisible";
    case HEALING: return "healing";
    case MFIND: return "monster detection";
    case TFIND: return "magic detection";
    case RAISE: return "raise level";
    case XHEAL: return "extra healing";
    case HASTE: return "haste self";
    case RESTORE: return "restore strength";
    case BLIND: return "blindness";
    case LEVIT: return "levitation";
    case NPOTIONS: error("Unknown subtype NPOTIONS");
  }
}

int Potion::worth(Potion::Type subtype) {
  switch (subtype) {
    case CONFUSION: return 5;
    case POISON: return 5;
    case STRENGTH: return 150;
    case SEEINVIS: return 100;
    case HEALING: return 130;
    case MFIND: return 130;
    case TFIND: return 105;
    case RAISE: return 250;
    case XHEAL: return 200;
    case HASTE: return 190;
    case RESTORE: return 130;
    case BLIND: return 5;
    case LEVIT: return 75;
    case NPOTIONS: error("Unknown subtype NPOTIONS");
  }
}

string& Potion::guess(Potion::Type subtype) {
  return guesses->at(static_cast<size_t>(subtype));
}

bool Potion::is_known(Potion::Type subtype) {
  return knowledge->at(static_cast<size_t>(subtype));
}

void Potion::set_known(Potion::Type subtype) {
  knowledge->at(static_cast<size_t>(subtype)) = true;
}

Potion::~Potion() {}

Potion::Potion() : Potion(random_potion_type()) {}

Potion::Potion(std::istream& data) { load(data); }

Potion::Potion(Potion::Type subtype_) : Item() {
  o_type = IO::Potion;
  o_which = subtype_;
  o_count = 1;
  set_attack_damage({1, 2});
  set_throw_damage({1, 2});

  subtype = subtype_;
}

Potion::Type Potion::get_type() const { return subtype; }

string Potion::get_description() const {
  stringstream os;

  if (Potion::is_known(subtype)) {
    if (o_count == 1) {
      os << "a potion of " << Potion::name(subtype);
    } else {
      os << to_string(o_count) << " potions of " << Potion::name(subtype);
    }

  } else {
    string const& color = colors->at(static_cast<size_t>(subtype));
    if (o_count == 1) {
      os << "a" << vowelstr(color) << " " << color << " potion";
    } else {
      os << to_string(o_count) << " " << color << " potions";
    }
  }

  string const& nickname{Potion::guess(subtype)};
  if (!nickname.empty()) { os << " {" << nickname << "}"; }

  return os.str();
}

void Potion::quaffed_by(Character& victim) {
  switch (static_cast<Potion::Type>(subtype)) {
    case CONFUSION: {
      if (&victim == player) { Potion::set_known(subtype); }
      victim.set_confused();
    } break;

    case POISON: {
      if (&victim == player) {
        Potion::set_known(subtype);
        player->become_poisoned();
      }
      // Currently, monsters cannot become poisoned. Perks of being a monster.
    } break;

    case STRENGTH: {
      if (&victim == player) {
        Potion::set_known(subtype);
        Game::io->message("you feel stronger, now.  What bulging muscles!");
      }
      victim.modify_strength(1);
    } break;

    case SEEINVIS: {
      victim.set_true_sight();
    } break;

    case HEALING: {
      if (&victim == player) {
        Potion::set_known(subtype);
        Game::io->message("you begin to feel better");
      }
      victim.restore_health(roll(victim.get_level(), 4), true);
      victim.set_not_blind();
    } break;

    case MFIND: {
      if (&victim == player) {
        Potion::set_known(subtype);
        player->set_sense_monsters();
      }
    } break;

    case TFIND: {
      if (&victim == player) {
        Potion::set_known(subtype);
        player->set_sense_magic();
      }
    } break;

    case RAISE: {
      if (&victim == player) { Potion::set_known(subtype); }
      victim.raise_level(1);
    } break;

    case XHEAL: {
      if (&victim == player) {
        Potion::set_known(subtype);
        Game::io->message("you begin to feel much better");
      }
      victim.restore_health(roll(victim.get_level(), 8), true);
      victim.set_not_blind();
    } break;

    case HASTE: {
      if (&victim == player) {
        Potion::set_known(subtype);
        player->increase_speed();
      }
    } break;

    case RESTORE: {
      victim.restore_strength();
    } break;

    case BLIND: {
      if (&victim == player) { Potion::set_known(subtype); }
      victim.set_blind();
    } break;

    case LEVIT: {
      if (&victim == player) { Potion::set_known(subtype); }
      victim.set_levitating();
    } break;

    case NPOTIONS: error("Unknown subtype NPOTIONS");
  }
}

bool potion_quaff_something(void) {
  Potion* obj{
      dynamic_cast<Potion*>(player->pack_find_item("quaff", IO::Potion))};
  if (obj == nullptr) {
    return false;

    // Make certain that it is somethings that we want to drink
  } else if (obj == nullptr || obj->o_type != IO::Potion) {
    Game::io->message("that's undrinkable");
    return false;
  }

  // Calculate the effect it has on the poor guy.
  bool const discardit{obj->o_count == 1};
  player->pack_remove(obj, false, false);

  obj->quaffed_by(*player);

  Game::io->refresh();

  string& nickname{Potion::guess(obj->get_type())};
  if (Potion::is_known(obj->get_type())) {
    nickname.clear();

  } else if (nickname.empty()) {
    Game::io->message("what do you want to call the potion?");
    nickname = Game::io->read_string();
  }

  /* Throw the item away */
  if (discardit) { delete obj; }
  return true;
}

void Potion::init_potions() {
  colors = new vector<string>;
  knowledge = new vector<bool>(Potion::NPOTIONS, false);
  guesses = new vector<string>(Potion::NPOTIONS, "");

  /* Pick a unique color for each potion */
  for (int i{0}; i < Potion::NPOTIONS; i++)
    for (;;) {
      size_t const color{os_rand_range(Color::max())};
      string const color_name{Color::get(color)};

      if (find(colors->cbegin(), colors->cend(), color_name) !=
          colors->cend()) {
        continue;
      }

      colors->push_back(color_name);
      break;
    }

  // Run some checks
  if (colors->size() != static_cast<size_t>(Potion::NPOTIONS)) {
    error("Potion init: wrong number of colors");
  } else if (knowledge->size() != static_cast<size_t>(Potion::NPOTIONS)) {
    error("Potion init: wrong number of knowledge");
  } else if (guesses->size() != static_cast<size_t>(Potion::NPOTIONS)) {
    error("Potion init: wrong number of guesses");
  }
}

void Potion::test_potions() {
  stringbuf buf;
  iostream test_data(&buf);

  vector<string> colors_{*colors};
  vector<bool> knowledge_{*knowledge};
  vector<string> guesses_{*guesses};

  save_potions(test_data);
  free_potions();
  load_potions(test_data);

  if (colors_ != *colors) { error("potion test 1 failed"); }
  if (knowledge_ != *knowledge) { error("potion test 2 failed"); }
  if (guesses_ != *guesses) { error("potion test 3 failed"); }
}

void Potion::save_potions(std::ostream& data) {
  Disk::save_tag(TAG_POTION, data);

  Disk::save(TAG_COLORS, colors, data);
  Disk::save(TAG_KNOWLEDGE, knowledge, data);
  Disk::save(TAG_GUESSES, guesses, data);

  Disk::save_tag(TAG_POTION, data);
}

void Potion::load_potions(std::istream& data) {
  size_t const npotions_size{static_cast<size_t>(Potion::NPOTIONS)};

  if (!Disk::load_tag(TAG_POTION, data)) { error("No potions found"); }

  if (!Disk::load(TAG_COLORS, colors, data)) { error("Potion tag error 1"); }
  if (colors->size() != npotions_size) { error("Potion size error 1") }

  if (!Disk::load(TAG_KNOWLEDGE, knowledge, data)) {
    error("Potion tag error 2");
  }
  if (knowledge->size() != npotions_size) { error("Potion size error 1") }

  if (!Disk::load(TAG_GUESSES, guesses, data)) { error("Potion tag error 3"); }
  if (guesses->size() != npotions_size) { error("Potion size error 1") }

  if (!Disk::load_tag(TAG_POTION, data)) { error("No potions end found"); }
}

void Potion::save(std::ostream& data) const {
  Item::save(data);
  static_assert(sizeof(Potion::Type) == sizeof(int), "Wrong Potion::Type size");
  Disk::save(TAG_POTION, static_cast<int>(subtype), data);
}

bool Potion::load(std::istream& data) {
  if (!Item::load(data) ||
      !Disk::load(TAG_POTION, reinterpret_cast<int&>(subtype), data)) {
    return false;
  }
  return true;
}

void Potion::free_potions() {
  delete colors;
  colors = nullptr;

  delete knowledge;
  knowledge = nullptr;

  delete guesses;
  guesses = nullptr;
}

void Potion::set_identified() { set_known(subtype); }

bool Potion::is_identified() const { return is_known(subtype); }

int Potion::get_base_value() const { return worth(subtype); }

int Potion::get_value() const {
  int value{get_base_value()};
  value *= o_count;
  if (!is_identified()) { value /= 2; }
  return value;
}

bool Potion::is_stackable() const { return true; }

bool Potion::autopickup() const {
  string const& guess_{guess(subtype)};
  if (!guess_.empty() && guess_.find('!') != string::npos) { return false; }
  return option_autopickup(o_type);
}
