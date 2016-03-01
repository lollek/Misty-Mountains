#include <vector>
#include <string>
#include <sstream>

#include "armor.h"
#include "disk.h"
#include "magic.h"
#include "error_handling.h"
#include "game.h"
#include "colors.h"
#include "coordinate.h"
#include "io.h"
#include "item.h"
#include "level.h"
#include "misc.h"
#include "monster.h"
#include "options.h"
#include "os.h"
#include "player.h"
#include "potions.h"
#include "rings.h"
#include "rogue.h"
#include "wand.h"
#include "weapons.h"

#include "scrolls.h"

using namespace std;

vector<string>* Scroll::guesses;
vector<bool>*   Scroll::knowledge;
vector<string>* Scroll::fake_name;

static Scroll::Type random_scroll_type() {
  vector<Scroll::Type> potential_scrolls;

  switch (Game::current_level) {
    default:

      [[clang::fallthrough]];
    case 12:
      potential_scrolls.push_back(Scroll::ENCHARMOR);
      potential_scrolls.push_back(Scroll::ENCH);

      [[clang::fallthrough]];
    case 10: case 11:
      potential_scrolls.push_back(Scroll::HOLD);
      potential_scrolls.push_back(Scroll::TELEP);
      potential_scrolls.push_back(Scroll::PROTECT);

      [[clang::fallthrough]];
    case 7: case 8: case 9:
      potential_scrolls.push_back(Scroll::REMOVE);

      [[clang::fallthrough]];
    case 5: case 6:
      potential_scrolls.push_back(Scroll::AGGR);
      potential_scrolls.push_back(Scroll::MAP);
      potential_scrolls.push_back(Scroll::CONFUSE);
      potential_scrolls.push_back(Scroll::SCARE);

      [[clang::fallthrough]];
    case 1: case 2: case 3: case 4:
      potential_scrolls.push_back(Scroll::SLEEP);
      potential_scrolls.push_back(Scroll::ID);
      potential_scrolls.push_back(Scroll::CREATE);
  }
  return potential_scrolls.at(os_rand_range(potential_scrolls.size()));
}

Scroll::~Scroll() {}

Scroll* Scroll::clone() const {
  return new Scroll(*this);
}

bool Scroll::is_magic() const {
  return true;
}

Scroll::Scroll(std::istream& data) {
  load(data);
}

Scroll::Scroll() : Scroll(random_scroll_type()) {}

Scroll::Scroll(Scroll::Type subtype_) : Item(), subtype(subtype_) {
  o_type = IO::Scroll;
  o_count = 1;
  o_which = subtype;
}

string Scroll::name(Scroll::Type subtype) {
  switch (subtype) {
    case CONFUSE:   return "monster confusion";
    case MAP:       return "magic mapping";
    case HOLD:      return "hold monster";
    case SLEEP:     return "sleep";
    case ENCHARMOR: return "enchant armor";
    case ID:        return "identify";
    case SCARE:     return "scare monster";
    case TELEP:     return "teleportation";
    case ENCH:      return "enchant weapon";
    case CREATE:    return "create monster";
    case REMOVE:    return "remove curse";
    case AGGR:      return "aggravate monsters";
    case PROTECT:   return "protect armor";
    case NSCROLLS:  error("Unknown subtype NSCROLLS");
  }
}

int Scroll::worth(Scroll::Type subtype) {
  switch (subtype) {
    case CONFUSE:   return 140;
    case MAP:       return 150;
    case HOLD:      return 180;
    case SLEEP:     return   5;
    case ENCHARMOR: return 160;
    case ID:        return 115;
    case SCARE:     return 200;
    case TELEP:     return 165;
    case ENCH:      return 150;
    case CREATE:    return  75;
    case REMOVE:    return 105;
    case AGGR:      return  20;
    case PROTECT:   return 250;
    case NSCROLLS:  error("Unknown subtype NSCROLLS");
  }
}

Scroll::Type Scroll::get_type() const {
  return subtype;
}

string& Scroll::guess(Scroll::Type subtype) {
  return guesses->at(static_cast<size_t>(subtype));
}

bool Scroll::is_known(Scroll::Type subtype) {
  return knowledge->at(static_cast<size_t>(subtype));
}

void Scroll::set_known(Scroll::Type subtype) {
  knowledge->at(static_cast<size_t>(subtype)) = true;
}

string Scroll::get_description() const {
  stringstream os;

  if (o_count == 1) {
    os << "a scroll";
  } else {
    os << to_string(o_count) << " scrolls";
  }

  bool prefix_added = false;
  if (Scroll::is_known(subtype)) {
    os << " of " << Scroll::name(subtype);
    prefix_added = true;
  }

  if (!Scroll::guess(subtype).empty()) {
    os << " {" << Scroll::guess(subtype) << "}";
    prefix_added = true;
  }

  if (!prefix_added) {
    os << " titled " << Scroll::fake_name->at(subtype);
  }

  return os.str();
}

void Scroll::init_scrolls() {
  fake_name = new vector<string>;
  knowledge = new vector<bool>(Scroll::NSCROLLS, false);
  guesses = new vector<string>(Scroll::NSCROLLS, "");

  vector<string> sylls {
    "a", "ab", "ag", "aks", "ala", "an", "app", "arg", "arze", "ash",
    "bek", "bie", "bit", "bjor", "blu", "bot", "bu", "byt", "comp",
    "con", "cos", "cre", "dalf", "dan", "den", "do", "e", "eep", "el",
    "eng", "er", "ere", "erk", "esh", "evs", "fa", "fid", "fri", "fu",
    "gan", "gar", "glen", "gop", "gre", "ha", "hyd", "i", "ing", "ip",
    "ish", "it", "ite", "iv", "jo", "kho", "kli", "klis", "la", "lech",
    "mar", "me", "mi", "mic", "mik", "mon", "mung", "mur", "nej",
    "nelg", "nep", "ner", "nes", "nes", "nih", "nin", "o", "od", "ood",
    "org", "orn", "ox", "oxy", "pay", "ple", "plu", "po", "pot",
    "prok", "re", "rea", "rhov", "ri", "ro", "rog", "rok", "rol", "sa",
    "san", "sat", "sef", "seh", "shu", "ski", "sna", "sne", "snik",
    "sno", "so", "sol", "sri", "sta", "sun", "ta", "tab", "tem",
    "ther", "ti", "tox", "trol", "tue", "turs", "u", "ulk", "um", "un",
    "uni", "ur", "val", "viv", "vly", "vom", "wah", "wed", "werg",
    "wex", "whon", "wun", "xo", "y", "yot", "yu", "zant", "zeb", "zim",
    "zok", "zon", "zum",
  };

  int const MAXNAME = 40;

  for (int i = 0; i < Scroll::NSCROLLS; i++) {
    string name;
    int num_words = os_rand_range(3) + 2;

    for (int j = 0; j < num_words; ++j) {
      int syllables = os_rand_range(3) + 1;

      for (int k = 0; k < syllables; ++k) {
        string const& syllable = sylls.at(os_rand_range(sylls.size()));
        if (name.size() + syllable.size() <= MAXNAME) {
          name += syllable;
        }
      }
      name += " ";
    }

    while (name.back() == ' ') {
      name.pop_back();
    }

    Scroll::fake_name->push_back(name);
  }


  // Run some checks
  size_t nscrolls_size = static_cast<size_t>(Scroll::NSCROLLS);
  if (fake_name->size() != nscrolls_size) { error("Scroll init: error 1"); }
  if (knowledge->size() != nscrolls_size) { error("Scroll init: error 2"); }
  if (guesses->size()   != nscrolls_size) { error("Scroll init: error 3"); }
}

void Scroll::test_scrolls() {
  stringbuf buf;
  iostream test_data(&buf);

  vector<string> fake_name_{*fake_name};
  vector<bool> knowledge_{*knowledge};
  vector<string> guesses_{*guesses};

  save_scrolls(test_data);
  free_scrolls();
  load_scrolls(test_data);

  if (fake_name_ != *fake_name) { error("scroll test 1 failed"); }
  if (knowledge_ != *knowledge) { error("scroll test 2 failed"); }
  if (guesses_ != *guesses)     { error("scroll test 3 failed"); }
}

void Scroll::save_scrolls(ostream& data) {
  Disk::save_tag(TAG_SCROLL, data);

  Disk::save(TAG_FAKE_NAME, fake_name, data);
  Disk::save(TAG_KNOWLEDGE, knowledge, data);
  Disk::save(TAG_GUESSES, guesses, data);

  Disk::save_tag(TAG_SCROLL, data);
}

void Scroll::load_scrolls(istream& data) {
  size_t nscrolls_size = static_cast<size_t>(Scroll::NSCROLLS);

  if (!Disk::load_tag(TAG_SCROLL, data))           { error("No scrolls found"); }

  if (!Disk::load(TAG_FAKE_NAME, fake_name, data)) { error("Scroll error 1"); }
  if (fake_name->size() != nscrolls_size)          { error("Scroll size error 1"); }

  if (!Disk::load(TAG_KNOWLEDGE, knowledge, data)) { error("Scroll error 2"); }
  if (knowledge->size() != nscrolls_size)          { error("Scroll size error 2"); }

  if (!Disk::load(TAG_GUESSES,   guesses, data))   { error("Scroll error 3"); }
  if (guesses->size() != nscrolls_size)            { error("Scroll size error 3"); }

  if (!Disk::load_tag(TAG_SCROLL, data))           { error("No scrolls end found"); }
}

void Scroll::free_scrolls() {
  delete fake_name;
  fake_name = nullptr;

  delete knowledge;
  knowledge = nullptr;

  delete guesses;
  guesses = nullptr;
}


void Scroll::save(std::ostream& data) const {
  Item::save(data);
  static_assert(sizeof(Scroll::Type) == sizeof(int), "Wrong Scroll::Type size");
  Disk::save(TAG_SCROLL, static_cast<int>(subtype), data);
}

bool Scroll::load(std::istream& data) {
  if (!Item::load(data) ||
      !Disk::load(TAG_SCROLL, reinterpret_cast<int&>(subtype), data)) {
    return false;
  }
  return true;
}

static bool enchant_players_armor() {
  class Armor* arm = player->equipped_armor();

  if (arm == nullptr) {
    switch (os_rand_range(3)) {
      case 0: Game::io->message("you are unsure if anything happened"); break;
      case 1: Game::io->message("you feel naked"); break;
      case 2: Game::io->message("you feel like something just touched you"); break;
    }
    return false;
  }

  arm->modify_armor(-1);
  arm->set_not_cursed();
  Game::io->message("your armor glows silver for a moment");
  return true;
}

static bool create_monster() {
  Coordinate const& player_pos = player->get_position();
  Coordinate mp;
  int i = 0;

  for (int y = player_pos.y - 1; y <= player_pos.y + 1; y++) {
    for (int x = player_pos.x - 1; x <= player_pos.x + 1; x++) {

      /* No duplicates */
      if ((y == player_pos.y && x == player_pos.x)) {
        continue;
      }

      /* Cannot stand there */
      if (!Game::level->can_step(x, y)) {
        continue;
      }

      /* Monsters cannot stand of scroll of stand monster */
      Item* item = Game::level->get_item(x, y);
      if (item != nullptr && item->o_type == IO::Scroll && item->o_which == Scroll::SCARE) {
        continue;
      }

      /* RNGsus doesn't want a monster here */
      if (os_rand_range(++i) != 0) {
        continue;
      }

      mp.y = y;
      mp.x = x;
    }
  }

  if (i == 0) {
    switch (os_rand_range(3)) {
      case 0: Game::io->message("you are unsure if anything happened"); break;
      case 1: Game::io->message("you hear a faint cry of anguish in the distance"); break;
      case 2: Game::io->message("you think you felt someone's presence"); break;
    }

  } else {
    Monster::Type mon_type = Monster::random_monster_type_for_level();
    Monster *monster = new Monster(mon_type, mp);
    Game::level->monsters.push_back(monster);
    Game::level->set_monster(mp, monster);
    Game::io->message("A " + monster->get_name() +
                      " appears out of thin air");
  }

  return i;
}

static bool player_enchant_weapon() {

  Item* weapon = player->equipped_weapon();
  if (weapon == nullptr) {
    switch (os_rand_range(2)) {
      case 0: Game::io->message("you feel a strange sense of loss"); break;
      case 1: Game::io->message("you are unsure if anything happened"); break;
    }
    return false;
  }

  weapon->set_not_cursed();
  if (os_rand_range(2) == 0) {
    weapon->modify_hit_plus(1);
  } else {
    weapon->modify_damage_plus(1);
  }

  stringstream os;
  os << "your "
     << Weapon::name(static_cast<Weapon::Type>(weapon->o_which))
     << " glows blue for a moment";
  Game::io->message(os.str());

  return true;
}

static void remove_curse() {
  player->pack_uncurse();
  Game::io->message("you feel as if somebody is watching over you");
}

static bool protect_armor() {

  class Armor* arm = player->equipped_armor();
  if (arm == nullptr) {
    switch (os_rand_range(2)) {
      case 0: Game::io->message("you feel a strange sense of loss"); break;
      case 1: Game::io->message("you are unsure if anything happened"); break;
    }
    return false;
  }

  arm->set_rustproof();
  stringstream os;
  os << "your armor is covered by a shimmering gold shield";
  Game::io->message(os.str());
  return true;
}

void Scroll::read() const {
  switch (subtype) {

    case Scroll::CONFUSE: {
      player->set_confusing_attack();
    } break;

    case Scroll::ENCHARMOR: {
      if (enchant_players_armor()) {
        set_known(Scroll::ENCHARMOR);
      }
    } break;

    case Scroll::HOLD: {
      if (magic_hold_nearby_monsters()) {
        set_known(Scroll::HOLD);
      }
    } break;

    case Scroll::SLEEP: {
      set_known(Scroll::SLEEP);
      player->fall_asleep();
    } break;

    case Scroll::CREATE: {
      if (create_monster()) {
        set_known(Scroll::CREATE);
      }
    } break;

    case Scroll::ID: {
      if (!is_known(Scroll::ID)) {
        stringstream os;
        os << "this scroll is an "
           << Scroll::name(Scroll::ID)
           << " scroll";
        Game::io->message(os.str());
      }
      set_known(Scroll::ID);
      player->pack_identify_item();
    } break;

    case Scroll::MAP: {
      set_known(Scroll::MAP);
      Game::io->message("this scroll has a map on it");
      Game::level->discover_map();
    } break;

    case Scroll::TELEP: {
      set_known(Scroll::TELEP);
      player->teleport(nullptr);
    } break;

    case Scroll::ENCH: {
      player_enchant_weapon();
    } break;

    case Scroll::SCARE: {
      /* Reading it is a mistake and produces laughter at her poor boo boo. */
      Game::io->message("you hear maniacal laughter in the distance");
    } break;

    case Scroll::REMOVE: {
      remove_curse();
    } break;

    case Scroll::AGGR: {
      /* This scroll aggravates all the monsters on the current
       * level and sets them running towards the hero */
      monster_aggravate_all();
      Game::io->message("you hear a high pitched humming noise");
    } break;

    case Scroll::PROTECT: {
      protect_armor();
    } break;

    case Scroll::NSCROLLS: error("Unknown scroll subtype NSCROLLS");
  }
}

void Scroll::set_identified() {
  set_known(subtype);
}

bool Scroll::is_identified() const {
  return is_known(subtype);
}

int Scroll::get_base_value() const {
  return worth(subtype);
}

int Scroll::get_value() const {
  int value = get_base_value();
  value *= o_count;
  if (!is_identified()) {
    value /= 2;
  }
  return value;
}

bool Scroll::is_stackable() const {
  return true;
}
bool Scroll::autopickup() const {
  string const& guess_ = guess(subtype);
  if (!guess_.empty() && guess_.find('!') != string::npos) {
    return false;
  }
  return option_autopickup(o_type);
}
