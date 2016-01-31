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
#include "things.h"
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

  for (size_t i = 0; i < things.size(); ++i)
  {
    wmove(tmp, static_cast<int>(i) + 1, 1);
    wprintw(tmp, "%c %s", index_to_char[i], things.at(i).oi_name.c_str());
  }

  wmove(stdscr, orig_pos.y, orig_pos.x);
  wrefresh(tmp);
  delwin(tmp);
}

int
wizard_list_items(void)
{
  io_msg("for what type of object do you want a list? ");
  print_things();

  int ch = io_readchar(true);
  touchwin(stdscr);
  refresh();

  pr_spec(static_cast<char>(ch));
  io_msg_clear();
  io_msg("--Press any key to continue--");
  io_readchar(false);

  io_msg_clear();
  touchwin(stdscr);
  return 0;
}

void wizard_create_item(void) {

  // Get itemtype
  io_msg("type of item: ");
  int type = io_readchar(true);
  io_msg_clear();

  switch (type) {

    // Supported types:
    case WEAPON: case ARMOR: case RING: case STICK: case GOLD:
    case POTION: case SCROLL: case TRAP: case FOOD: {
      break;
    }

    default: {
      io_msg("Bad pick");
      return;
    }
  }

  // Get item subtype
  io_msg("which %c do you want? (0-f)", type);
  char ch = io_readchar(true);
  int which = isdigit(ch) ? ch - '0' : ch - 'a' + 10;
  io_msg_clear();

  // Allocate item
  Item* obj = nullptr;
  switch (type) {
    case TRAP: {
      if (which < 0 || which >= NTRAPS) {
        io_msg("Bad trap id");

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

      io_msg("blessing? (+,-,n)");
      char bless = io_readchar(true);
      io_msg_clear();

      if (bless == '-') {
        obj->o_flags |= ISCURSED;
        obj->modify_hit_plus(-os_rand_range(3) + 1);

      } else if (bless == '+')
        obj->modify_hit_plus(os_rand_range(3) + 1);
    } break;

    case ARMOR: {
      obj = new Armor(false);

      io_msg("blessing? (+,-,n)");
      char bless = io_readchar(true);
      io_msg_clear();
      if (bless == '-') {
        obj->o_flags |= ISCURSED;
        obj->modify_armor(os_rand_range(3) + 1);

      } else if (bless == '+')
        obj->modify_armor(-os_rand_range(3) + 1);
    } break;

    case RING: {
      obj = new Ring(static_cast<Ring::Type>(which), false);

      switch (obj->o_which) {
        case Ring::Type::PROTECT: case Ring::Type::ADDSTR:
        case Ring::Type::ADDHIT:  case Ring::Type::ADDDAM: {
          io_msg("blessing? (+,-,n)");
          char bless = io_readchar(true);
          io_msg_clear();
          if (bless == '-')
            obj->o_flags |= ISCURSED;
          obj->set_armor(bless == '-' ? -1 : os_rand_range(2) + 1);
        } break;
      }
    } break;

    case GOLD: {
      char buf[MAXSTR] = { '\0' };
      io_msg("how much?");
      int amount = 0;
      if (io_readstr(buf) == 0) {
        amount = static_cast<short>(atoi(buf));
      }
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
  wclear(hw);

  for (int y = 1; y < NUMLINES - 1; y++)  {
    for (int x = 0; x < NUMCOLS; x++) {
      chtype ch = static_cast<chtype>(Game::level->get_ch(x, y));

      if (!Game::level->is_real(x, y)) {
        ch |= A_STANDOUT;
      }

      mvwaddcch(hw, y, x, ch);
    }
  }
  show_win("---More (level map)---");
}

void wizard_levels_and_gear(void) {
  player->raise_level(9);

  /* Give him a sword (+1,+1) */
  if (pack_equipped_item(EQUIPMENT_RHAND) == nullptr) {
    Item* obj = new Weapon(Weapon::TWOSWORD, false);
    obj->set_hit_plus(1);
    obj->set_damage_plus(1);
    pack_equip_item(obj);
  }

  /* And his suit of armor */
  if (pack_equipped_item(EQUIPMENT_ARMOR) == nullptr) {
    Item* obj = new Armor(Armor::Type::PLATE_MAIL, false);
    obj->set_armor(-5);
    obj->o_flags |= ISKNOW;
    obj->o_count = 1;
    pack_equip_item(obj);
  }
}
