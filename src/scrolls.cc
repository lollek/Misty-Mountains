#include <vector>
#include <string>
#include <sstream>

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
#include "pack.h"
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
  int value = os_rand_range(100);

  int end = static_cast<int>(Scroll::Type::NSCROLLS);
  for (int i = 0; i < end; ++i) {
    Scroll::Type type = static_cast<Scroll::Type>(i);
    int probability = Scroll::probability(type);

    if (value < probability) {
      return type;

    } else {
      value -= probability;
    }
  }

  error("Error! Sum of probabilities is not 100%");
}

Scroll::~Scroll() {}

Scroll* Scroll::clone() const {
  return new Scroll(*this);
}

bool Scroll::is_magic() const {
  return true;
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
    case FDET:      return "food detection";
    case TELEP:     return "teleportation";
    case ENCH:      return "enchant weapon";
    case CREATE:    return "create monster";
    case REMOVE:    return "remove curse";
    case AGGR:      return "aggravate monsters";
    case PROTECT:   return "protect armor";
    case NSCROLLS:  error("Unknown subtype NSCROLLS");
  }
}

int Scroll::probability(Scroll::Type subtype) {
  switch (subtype) {
    case CONFUSE:   return  7;
    case MAP:       return  4;
    case HOLD:      return  2;
    case SLEEP:     return  3;
    case ENCHARMOR: return  7;
    case ID:        return 43;
    case SCARE:     return  3;
    case FDET:      return  2;
    case TELEP:     return  5;
    case ENCH:      return  8;
    case CREATE:    return  4;
    case REMOVE:    return  7;
    case AGGR:      return  3;
    case PROTECT:   return  2;
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
    case FDET:      return  60;
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

  if (Scroll::is_known(subtype)) {
    os << " of " << Scroll::name(subtype);
  } else if (!Scroll::guess(subtype).empty()) {
    os << " called " << Scroll::guess(subtype);
  } else {
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
  if (fake_name->size() != static_cast<size_t>(Scroll::NSCROLLS)) {
    error("Scroll init: wrong number of fake names");
  } else if (knowledge->size() != static_cast<size_t>(Scroll::NSCROLLS)) {
    error("Scroll init: wrong number of knowledge");
  } else if (guesses->size() != static_cast<size_t>(Scroll::NSCROLLS)) {
    error("Scroll init: wrong number of guesses");
  }
}

void Scroll::free_scrolls() {
  delete guesses;
  guesses = nullptr;

  delete knowledge;
  knowledge = nullptr;

  delete fake_name;
  fake_name = nullptr;
}

static bool enchant_players_armor() {
  Item* arm = pack_equipped_item(EQUIPMENT_ARMOR);

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
    Monster *monster = new Monster(Monster::random_monster_type(), mp, Game::level->get_room(mp));
    Game::level->monsters.push_back(monster);
    Game::level->set_monster(mp, monster);
    Game::io->message("A " + monster->get_name() +
                      " appears out of thin air");
  }

  return i;
}

static bool food_detection() {
  bool food_seen = false;
  wclear(Game::io->extra_screen);

  for (Item const* obj : Game::level->items) {
    if (obj->o_type == IO::Food) {
      food_seen = true;
      mvwaddcch(Game::io->extra_screen, obj->get_y(), obj->get_x(), IO::Food);
    }
  }

  if (food_seen) {
    Game::io->show_extra_screen("Your nose tingles and you smell food.--More--");
  } else {
    Game::io->message("your nose tingles");
  }

  return food_seen;
}

static bool player_enchant_weapon() {

  Item* weapon = pack_equipped_item(EQUIPMENT_RHAND);
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
  for (int i = 0; i < NEQUIPMENT; ++i) {
    if (pack_equipped_item(static_cast<equipment_pos>(i)) != nullptr) {
      pack_equipped_item(static_cast<equipment_pos>(i))->set_not_cursed();
    }
  }

  Game::io->message("you feel as if somebody is watching over you");
}

static bool protect_armor() {

  Item* arm = pack_equipped_item(EQUIPMENT_ARMOR);
  if (arm == nullptr) {
    switch (os_rand_range(2)) {
      case 0: Game::io->message("you feel a strange sense of loss"); break;
      case 1: Game::io->message("you are unsure if anything happened"); break;
    }
    return false;
  }

  arm->o_flags |= ISPROT;
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
      pack_identify_item();
    } break;

    case Scroll::MAP: {
      set_known(Scroll::MAP);
      Game::io->message("this scroll has a map on it");
      Game::io->print_level_layout();
    } break;

    case Scroll::FDET: {
      if (food_detection()) {
        set_known(Scroll::FDET);
      }
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
