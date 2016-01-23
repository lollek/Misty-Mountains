#include <vector>

using namespace std;

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

bool wizard = false;

static void
pr_spec(char type)
{
  WINDOW* printscr = dupwin(stdscr);

  Coordinate orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  vector<obj_info>* ptr;
  size_t max;
  switch (type)
  {
    case POTION: ptr = nullptr;            max = Potion::Type::NPOTIONS;  break;
    case SCROLL: ptr = &scroll_info;       max = NSCROLLS;  break;
    case RING:   ptr = nullptr;            max = Ring::Type::NRINGS;    break;
    case STICK:  ptr = nullptr;            max = Wand::Type::NWANDS; break;
    case ARMOR:  ptr = nullptr;            max = Armor::Type::NARMORS;   break;
    case WEAPON: ptr = &weapon_info;       max = MAXWEAPONS;break;
    default:     ptr = nullptr;            max = 0;         break;
  }

  char ch = '0';
  for (size_t i = 0; i < max; ++i)
  {
    string name;
    size_t prob;
    wmove(printscr, static_cast<int>(i) + 1, 1);

    switch (type) {
      case ARMOR: {
        name = Armor::name(static_cast<Armor::Type>(i));
        prob = static_cast<size_t>(Armor::probability(static_cast<Armor::Type>(i)));
      } break;

      case POTION: {
        name = Potion::name(static_cast<Potion::Type>(i));
        prob = static_cast<size_t>(Potion::probability(static_cast<Potion::Type>(i)));
      } break;

      case STICK: {
        name = Wand::name(static_cast<Wand::Type>(i));
        prob = static_cast<size_t>(Wand::probability(static_cast<Wand::Type>(i)));
      } break;

      case RING: {
        name = Ring::name(static_cast<Ring::Type>(i));
        prob = static_cast<size_t>(Ring::probability(static_cast<Ring::Type>(i)));
      } break;

      default: {
        name = ptr->at(i).oi_name;
        prob = ptr->at(i).oi_prob;
      } break;
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

void
wizard_create_item(void)
{
  io_msg("type of item: ");
  int type = io_readchar(true);
  io_msg_clear();

  if (!(type == WEAPON || type == ARMOR || type == RING || type == STICK
      || type == GOLD || type == POTION || type == SCROLL || type == TRAP))
  {
    io_msg("Bad pick");
    return;
  }

  io_msg("which %c do you want? (0-f)", type);
  char ch = io_readchar(true);
  int which = isdigit(ch) ? ch - '0' : ch - 'a' + 10;
  io_msg_clear();

  Item* obj = nullptr;
  switch (type)
  {
    case TRAP:
      {
        if (which < 0 || which >= NTRAPS)
          io_msg("Bad trap id");
        else
        {
          Game::level->set_not_real(player->get_position());
          Game::level->set_trap_type(player->get_position(), static_cast<size_t>(which));
        }
        return;
      }
    case STICK: obj = new Wand(static_cast<Wand::Type>(which)); break;
    case SCROLL: obj = scroll_create(which); break;
    case POTION: {
      obj = new Potion(static_cast<Potion::Type>(which));
      obj->o_count = 10;
    } break;
    case FOOD: obj = new_food(which); break;
    case WEAPON:
      {
        obj = weapon_create(which, false);

        io_msg("blessing? (+,-,n)");
        char bless = io_readchar(true);
        io_msg_clear();

        if (bless == '-')
        {
          obj->o_flags |= ISCURSED;
          obj->o_hplus -= os_rand_range(3) + 1;
        }
        else if (bless == '+')
          obj->o_hplus += os_rand_range(3) + 1;
      }
      break;

    case ARMOR:
      {
        obj = new Armor(false);

        io_msg("blessing? (+,-,n)");
        char bless = io_readchar(true);
        io_msg_clear();
        if (bless == '-')
        {
          obj->o_flags |= ISCURSED;
          obj->o_arm += os_rand_range(3) + 1;
        }
        else if (bless == '+')
          obj->o_arm -= os_rand_range(3) + 1;
      }
      break;

    case RING:
      {
        obj = new Ring(static_cast<Ring::Type>(which), false);
        switch (obj->o_which)
        {
          case Ring::Type::PROTECT: case Ring::Type::ADDSTR:
          case Ring::Type::ADDHIT:  case Ring::Type::ADDDAM:
            io_msg("blessing? (+,-,n)");
            char bless = io_readchar(true);
            io_msg_clear();
            if (bless == '-')
              obj->o_flags |= ISCURSED;
            obj->o_arm = (bless == '-' ? -1 : os_rand_range(2) + 1);
            break;
        }
      }
      break;

    case GOLD:
      {
        char buf[MAXSTR] = { '\0' };
        io_msg("how much?");
        if (io_readstr(buf) == 0)
          obj->o_goldval = static_cast<short>(atoi(buf));
      }
      break;

    default:
      error("Unimplemented item: " + to_string(which));
  }

  if (obj == nullptr) {
    error("object was null");
  }
  pack_add(obj, false);
}

void
wizard_show_map(void)
{
  wclear(hw);
  for (int y = 1; y < NUMLINES - 1; y++)
    for (int x = 0; x < NUMCOLS; x++)
    {
      if (!Game::level->is_real(x, y))
        wstandout(hw);
      wmove(hw, y, x);
      waddcch(hw, static_cast<chtype>(Game::level->get_ch(x, y)));
      if (!Game::level->is_real(x, y))
        wstandend(hw);
    }
  show_win("---More (level map)---");
}

void
wizard_levels_and_gear(void)
{
  player->raise_level(9);

  /* Give him a sword (+1,+1) */
  if (pack_equipped_item(EQUIPMENT_RHAND) == nullptr)
  {
    Item* obj = weapon_create(TWOSWORD, false);
    obj->o_hplus = 1;
    obj->o_dplus = 1;
    pack_equip_item(obj);
  }

  /* And his suit of armor */
  if (pack_equipped_item(EQUIPMENT_ARMOR) == nullptr)
  {
    Item* obj = new Armor(Armor::Type::PLATE_MAIL, false);
    obj->o_arm = -5;
    obj->o_flags |= ISKNOW;
    obj->o_count = 1;
    pack_equip_item(obj);
  }
}
