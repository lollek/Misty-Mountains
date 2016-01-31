#include <vector>
#include <string>
#include <sstream>

#include "error_handling.h"
#include "io.h"
#include "pack.h"
#include "level.h"
#include "misc.h"
#include "player.h"
#include "monster.h"
#include "colors.h"
#include "os.h"
#include "rogue.h"
#include "item.h"
#include "game.h"

#include "potions.h"

using namespace std;

std::vector<std::string>        Potion::guesses;
std::vector<bool>               Potion::knowledge;
std::vector<std::string const*> Potion::colors;

static Potion::Type random_potion_type() {
  int value = os_rand_range(100);

  int end = static_cast<int>(Potion::Type::NPOTIONS);
  for (int i = 0; i < end; ++i) {
    Potion::Type type = static_cast<Potion::Type>(i);
    int probability = Potion::probability(type);

    if (value < probability) {
      return type;

    } else {
      value -= probability;
    }
  }

  error("Error! Sum of probabilities is not 100%");
}

Potion* Potion::clone() const {
  return new Potion(*this);
}

bool Potion::is_magic() const {
  return true;
}

string Potion::name(Potion::Type subtype) {
  switch(subtype) {
    case CONFUSION: return "confusion";
    case LSD:       return "hallucination";
    case POISON:    return "poison";
    case STRENGTH:  return "gain strength";
    case SEEINVIS:  return "see invisible";
    case HEALING:   return "healing";
    case MFIND:     return "monster detection";
    case TFIND:     return "magic detection";
    case RAISE:     return "raise level";
    case XHEAL:     return "extra healing";
    case HASTE:     return "haste self";
    case RESTORE:   return "restore strength";
    case BLIND:     return "blindness";
    case LEVIT:     return "levitation";
    case NPOTIONS:  error("Unknown subtype NPOTIONS");
  }
}

int Potion::probability(Potion::Type subtype) {
  switch(subtype) {
    case CONFUSION: return 7;
    case LSD:       return 8;
    case POISON:    return 8;
    case STRENGTH:  return 13;
    case SEEINVIS:  return 3;
    case HEALING:   return 13;
    case MFIND:     return 6;
    case TFIND:     return 6;
    case RAISE:     return 2;
    case XHEAL:     return 5;
    case HASTE:     return 5;
    case RESTORE:   return 13;
    case BLIND:     return 5;
    case LEVIT:     return 6;
    case NPOTIONS:  error("Unknown subtype NPOTIONS");
  }
}

int Potion::worth(Potion::Type subtype) {
  switch(subtype) {
    case CONFUSION: return 5;
    case LSD:       return 5;
    case POISON:    return 5;
    case STRENGTH:  return 150;
    case SEEINVIS:  return 100;
    case HEALING:   return 130;
    case MFIND:     return 130;
    case TFIND:     return 105;
    case RAISE:     return 250;
    case XHEAL:     return 200;
    case HASTE:     return 190;
    case RESTORE:   return 130;
    case BLIND:     return 5;
    case LEVIT:     return 75;
    case NPOTIONS:  error("Unknown subtype NPOTIONS");
  }
}

string& Potion::guess(Potion::Type subtype) {
  return guesses.at(static_cast<size_t>(subtype));
}

bool Potion::is_known(Potion::Type subtype) {
  return knowledge.at(static_cast<size_t>(subtype));
}

void Potion::set_known(Potion::Type subtype) {
  knowledge.at(static_cast<size_t>(subtype)) = true;
}

Potion::~Potion() {}

Potion::Potion() : Potion(random_potion_type()) {}

Potion::Potion(Potion::Type subtype_) : Item() {
  o_type       = POTION;
  o_which      = subtype_;
  o_count      = 1;
  o_damage     = {1, 2};
  o_hurldmg    = {1, 2};

  subtype = subtype_;
  guesses.resize(static_cast<size_t>(Potion::NPOTIONS));
  knowledge.resize(static_cast<size_t>(Potion::NPOTIONS));
}

Potion::Type Potion::get_type() const {
  return subtype;
}

string Potion::get_description() const {
  stringstream os;

  if (Potion::is_known(subtype)) {
    if (o_count == 1) {
      os << "A potion of " << Potion::name(subtype);
    } else {
      os << to_string(o_count) << " potions of " << Potion::name(subtype);
    }

  } else {
    string const& color = *colors.at(static_cast<size_t>(subtype));
    if (o_count == 1) {
      os << "A" << vowelstr(color) << " " << color << " potion";
    } else {
      os << to_string(o_count) << " " << color << " potions";
    }

    string const& nickname = Potion::guess(subtype);
    if (!nickname.empty()) {
      os << " called " << nickname;
    }
  }

  return os.str();
}

void Potion::quaffed_by(Character& victim) {
  switch(static_cast<Potion::Type>(subtype)) {
    case CONFUSION: {
      if (&victim == player && !player->is_hallucinating()) {
        Potion::set_known(subtype);
      }
      victim.set_confused();
    } break;

    case LSD: {
      if (&victim == player) {
        Potion::set_known(subtype);
      }
      victim.set_hallucinating();
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
        io_msg("you feel stronger, now.  What bulging muscles!");
      }
      victim.modify_strength(1);
    } break;

    case SEEINVIS:  {
      victim.set_true_sight();
    } break;

    case HEALING: {
      if (&victim == player) {
        Potion::set_known(subtype);
        io_msg("you begin to feel better");
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
        bool show = false;
        if (!Game::level->items.empty()) {
          wclear(hw);
          for (Item* item : Game::level->items) {
            if (item->is_magic()) {
              Potion::set_known(subtype);
              show = true;
              mvwaddcch(hw, item->get_y(), item->get_x(), MAGIC);
            }
          }

          if (monster_show_if_magic_inventory()) {
            show = true;
          }
        }

        if (show) {
          Potion::set_known(subtype);
          show_win("You sense the presence of magic on this level.--More--");
        } else {
          io_msg("you have a strange feeling for a moment, then it passes");
        }
      }
    } break;

    case RAISE: {
      if (&victim == player) {
        Potion::set_known(subtype);
      }
      victim.raise_level(1);
    } break;

    case XHEAL: {
      if (&victim == player) {
        Potion::set_known(subtype);
        io_msg("you begin to feel much better");
      }
      victim.restore_health(roll(victim.get_level(), 8), true);
      victim.set_not_blind();
      victim.set_not_hallucinating();
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
      if (&victim == player) {
        Potion::set_known(subtype);
      }
      victim.set_blind();
    } break;

    case LEVIT: {
      if (&victim == player) {
        Potion::set_known(subtype);
      }
      victim.set_levitating();
    } break;

    case NPOTIONS:  error("Unknown subtype NPOTIONS");
  }
}

bool
potion_quaff_something(void)
{
  Potion* obj = dynamic_cast<Potion*>(pack_get_item("quaff", POTION));
  if (obj == nullptr) {
    return false;

  // Make certain that it is somethings that we want to drink
  } else if (obj == nullptr || obj->o_type != POTION) {
    io_msg("that's undrinkable");
    return false;
  }

  // Calculate the effect it has on the poor guy.
  bool discardit = obj->o_count == 1;
  pack_remove(obj, false, false);

  obj->quaffed_by(*player);

  io_refresh_statusline();

  string& nickname = Potion::guess(obj->get_type());
  if (Potion::is_known(obj->get_type())) {
    nickname.clear();

  } else if (nickname.empty()) {
    char tmpbuf[MAXSTR] = { '\0' };
    io_msg("what do you want to call the potion? ");
    if (io_readstr(tmpbuf) == 0) {
      nickname = tmpbuf;
    }
  }

  /* Throw the item away */
  if (discardit) {
    delete obj;
  }
  return true;
}

string potion_description(Item const* item) {
  Potion const* potion = dynamic_cast<Potion const*>(item);
  if (potion == nullptr) {
    error("Cannot describe non-potion as potion");
  }
  return potion->get_description();
}

void Potion::init_potions() {
  knowledge = vector<bool>(Potion::NPOTIONS, false);
  guesses = vector<string>(Potion::NPOTIONS, "");

  /* Pick a unique color for each potion */
  for (int i = 0; i < Potion::NPOTIONS; i++)
    for (;;) {
      size_t color = os_rand_range(color_max());

      if (find(colors.cbegin(), colors.cend(), &color_get(color)) !=
          colors.cend()) {
        continue;
      }

      colors.push_back(&color_get(color));
      break;
    }

  // Run some checks
  if (colors.size() != static_cast<size_t>(Potion::NPOTIONS)) {
    error("Potion init: wrong number of colors");
  } else if (knowledge.size() != static_cast<size_t>(Potion::NPOTIONS)) {
    error("Potion init: wrong number of knowledge");
  } else if (guesses.size() != static_cast<size_t>(Potion::NPOTIONS)) {
    error("Potion init: wrong number of guesses");
  }

}


