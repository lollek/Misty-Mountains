#include <vector>

#include "food.h"
#include "gold.h"
#include "error_handling.h"
#include "game.h"
#include "coordinate.h"
#include "potions.h"
#include "scrolls.h"
#include "command.h"
#include "options.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "rings.h"
#include "misc.h"
#include "level.h"
#include "weapons.h"
#include "player.h"
#include "wand.h"
#include "traps.h"
#include "os.h"
#include "rogue.h"

#include "wizard.h"

using namespace std;

bool wizard = false;

static void
pr_spec(char type)
{
  WINDOW* printscr = dupwin(stdscr);

  Coordinate orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  size_t max;
  switch (type)
  {
    case POTION: max = Potion::Type::NPOTIONS;  break;
    case SCROLL: max = Scroll::Type::NSCROLLS;  break;
    case RING:   max = Ring::Type::NRINGS;    break;
    case STICK:  max = Wand::Type::NWANDS; break;
    case ARMOR:  max = Armor::Type::NARMORS;   break;
    case WEAPON: max = Weapon::NWEAPONS; break;
    default: error("Unknown type in pr_spec");
  }

  char ch = '0';
  for (size_t i = 0; i < max; ++i)
  {
    string name;
    int prob;
    wmove(printscr, static_cast<int>(i) + 1, 1);

    switch (type) {
      case SCROLL: {
        name = Scroll::name(static_cast<Scroll::Type>(i));
        prob = Scroll::probability(static_cast<Scroll::Type>(i));
      } break;

      case ARMOR: {
        name = Armor::name(static_cast<Armor::Type>(i));
        prob = Armor::probability(static_cast<Armor::Type>(i));
      } break;

      case POTION: {
        name = Potion::name(static_cast<Potion::Type>(i));
        prob = Potion::probability(static_cast<Potion::Type>(i));
      } break;

      case STICK: {
        name = Wand::name(static_cast<Wand::Type>(i));
        prob = Wand::probability(static_cast<Wand::Type>(i));
      } break;

      case RING: {
        name = Ring::name(static_cast<Ring::Type>(i));
        prob = Ring::probability(static_cast<Ring::Type>(i));
      } break;

      case WEAPON: {
        name = Weapon::name(static_cast<Weapon::Type>(i));
        prob = Weapon::probability(static_cast<Weapon::Type>(i));
      } break;

      default: error("Unknown type in pr_spec");
    }

    wprintw(printscr, "%c: %s (%d%%)", ch, name.c_str(), prob);
    ch = ch == '9' ? 'a' : (ch + 1);
  }

  wmove(stdscr, orig_pos.y, orig_pos.x);
  wrefresh(printscr);
  delwin(printscr);
}

static void
print_things(void)
{
  char index_to_char[] = { POTION, SCROLL, FOOD, WEAPON, ARMOR, RING, STICK };
  WINDOW* tmp = dupwin(stdscr);

  Coordinate orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  for (int i = 0; i < static_cast<int>(Item::NITEMS); ++i)
  {
    wmove(tmp, i + 1, 1);
    wprintw(tmp, "%c %s", index_to_char[i], Item::name(static_cast<Item::Type>(i)).c_str());
  }

  wmove(stdscr, orig_pos.y, orig_pos.x);
  wrefresh(tmp);
  delwin(tmp);
}

int
wizard_list_items(void)
{
  Game::io->message("for what type of object do you want a list? ");
  print_things();

  int ch = io_readchar(true);
  touchwin(stdscr);
  refresh();

  pr_spec(static_cast<char>(ch));
  Game::io->clear_message();
  Game::io->message("--Press any key to continue--");
  io_readchar(false);

  Game::io->clear_message();
  touchwin(stdscr);
  return 0;
}

void wizard_create_item(void) {

  // Get itemtype
  Game::io->message("type of item: ");
  int type = io_readchar(true);
  Game::io->clear_message();

  switch (type) {

    // Supported types:
    case WEAPON: case ARMOR: case RING: case STICK: case GOLD:
    case POTION: case SCROLL: case TRAP: case FOOD: {
      break;
    }

    default: {
      Game::io->message("Bad pick");
      return;
    }
  }

  // Get item subtype
  Game::io->message("which " + string(1, type) + " do you want? (0-f)");
  char ch = io_readchar(true);
  int which = isdigit(ch) ? ch - '0' : ch - 'a' + 10;
  Game::io->clear_message();

  // Allocate item
  Item* obj = nullptr;
  switch (type) {
    case TRAP: {
      if (which < 0 || which >= Trap::NTRAPS) {
        Game::io->message("Bad trap id");

      } else {
        Game::level->set_not_real(player->get_position());
        Game::level->set_trap_type(player->get_position(), static_cast<size_t>(which));
      }
    } return;

    case STICK: {
      obj = new Wand(static_cast<Wand::Type>(which));
    } break;

    case SCROLL: {
      obj = new Scroll(static_cast<Scroll::Type>(which));
      obj->o_count = 10;
    } break;

    case POTION: {
      obj = new Potion(static_cast<Potion::Type>(which));
      obj->o_count = 10;
    } break;

    case FOOD: {
      obj = new Food(static_cast<Food::Type>(which));
    } break;

    case WEAPON: {
      obj = new Weapon(static_cast<Weapon::Type>(which), false);

      Game::io->message("blessing? (+,-,n)");
      char bless = io_readchar(true);
      Game::io->clear_message();

      if (bless == '-') {
        obj->set_cursed();
        obj->modify_hit_plus(-os_rand_range(3) + 1);

      } else if (bless == '+')
        obj->modify_hit_plus(os_rand_range(3) + 1);
    } break;

    case ARMOR: {
      obj = new Armor(false);

      Game::io->message("blessing? (+,-,n)");
      char bless = io_readchar(true);
      Game::io->clear_message();
      if (bless == '-') {
        obj->set_cursed();
        obj->modify_armor(os_rand_range(3) + 1);

      } else if (bless == '+')
        obj->modify_armor(-os_rand_range(3) + 1);
    } break;

    case RING: {
      obj = new Ring(static_cast<Ring::Type>(which), false);

      switch (obj->o_which) {
        case Ring::Type::PROTECT: case Ring::Type::ADDSTR:
        case Ring::Type::ADDHIT:  case Ring::Type::ADDDAM: {
          Game::io->message("blessing? (+,-,n)");
          char bless = io_readchar(true);
          Game::io->clear_message();
          if (bless == '-')
            obj->set_cursed();
          obj->set_armor(bless == '-' ? -1 : os_rand_range(2) + 1);
        } break;
      }
    } break;

    case GOLD: {
      Game::io->message("how much?");
      int amount = stoi(Game::io->read_string());
      obj = new Gold(amount);
    } break;

    default: {
      error("Unimplemented item: " + to_string(which));
    }
  }

  if (obj == nullptr) {
    error("object was null");
  }
  pack_add(obj, false);
}

void wizard_show_map(void) {
  wclear(Game::io->extra_screen);

  for (int y = 1; y < NUMLINES - 1; y++)  {
    for (int x = 0; x < NUMCOLS; x++) {
      chtype ch = static_cast<chtype>(Game::level->get_ch(x, y));

      if (!Game::level->is_real(x, y)) {
        ch |= A_STANDOUT;
      }

      mvwaddcch(Game::io->extra_screen, y, x, ch);
    }
  }
  Game::io->show_extra_screen("---More (level map)---");
}

void wizard_levels_and_gear(void) {
  player->raise_level(9);

  /* Give him a sword (+1,+1) */
  if (pack_equipped_item(EQUIPMENT_RHAND) == nullptr) {
    Weapon* weapon = new Weapon(Weapon::TWOSWORD, false);
    weapon->set_hit_plus(1);
    weapon->set_damage_plus(1);
    weapon->set_identified();
    pack_equip_item(weapon);
  }

  /* And his suit of armor */
  if (pack_equipped_item(EQUIPMENT_ARMOR) == nullptr) {
    Armor* armor = new Armor(Armor::Type::PLATE_MAIL, false);
    armor->set_armor(-5);
    armor->set_identified();
    pack_equip_item(armor);
  }
}
