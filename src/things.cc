#include <vector>
#include <string>
#include <sstream>

using namespace std;

#include "game.h"
#include "coordinate.h"
#include "potions.h"
#include "scrolls.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "rings.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "weapons.h"
#include "wand.h"
#include "os.h"
#include "options.h"
#include "rogue.h"

#include "things.h"

/* Only oi_prob is used */
vector<obj_info> things = {
/*   oi_name oi_prob oi_worth oi_guess oi_know */
    { "potion",	26,	0,	"",	false },
    { "scroll",	36,	0,	"",	false },
    { "food",	16,	0,	"",	false },
    { "weapon",	 7,	0,	"",	false },
    { "armor",	 7,	0,	"",	false },
    { "ring",	 4,	0,	"",	false },
    { "stick",	 4,	0,	"",	false },
};

string
inv_name(Item const* item, bool drop)
{
  stringstream buffer;

  switch (item->o_type)
  {
    case POTION: {
      char buf[MAXSTR];
      potion_description(item, buf);
      buffer << buf;
    } break;
    case RING: {
      char buf[MAXSTR];
      ring_description(item, buf);
      buffer << buf;
    } break;
    case STICK: {
      char buf[MAXSTR];
      wand_description(item, buf);
      buffer << buf;
    } break;
    case SCROLL: {
      char buf[MAXSTR];
      scroll_description(item, buf);
      buffer << buf;
    } break;
    case WEAPON: buffer << weapon_description(item); break;
    case AMMO: buffer << weapon_description(item); break;
    case ARMOR: buffer << armor_description(item); break;
    case FOOD: {
      string obj_type = item->o_which == 1 ? "fruit" : "food ration";
      if (item->o_count == 1) {
        buffer << "A " << obj_type;
      } else {
        buffer << item->o_count << " " << obj_type;
      }
    } break;
    case AMULET: buffer << "The Amulet of Yendor"; break;
    case GOLD: buffer << item->o_goldval << " Gold pieces"; break;

    default:
      io_msg("You feel a disturbance in the force");
      buffer << "Something bizarre " << unctrl(static_cast<chtype>(item->o_type));
      break;
  }

  string return_value = buffer.str();
  return_value.at(0) = static_cast<char>(drop
      ? tolower(return_value.at(0))
      : toupper(return_value.at(0)));
  return return_value;
}

Item*
new_food(int which)
{
  /* Reset levels-without-food counter */
  Game::levels_without_food = 0;

  Item* cur = new Item();
  cur->o_count = 1;
  cur->o_type = FOOD;
  switch (which)
  {
    case 0: case 1: cur->o_which = which; break;
    default: cur->o_which = os_rand_range(10) ? 0 : 1; break;
  }
  return cur;
}

Item*
new_amulet(void)
{
  Item* obj       = new Item();
  obj->o_damage  = {1, 2};
  obj->o_hurldmg = {1, 2};
  obj->o_type    = AMULET;

  return obj;
}

Item*
new_thing(void)
{
  /* Decide what kind of object it will be
   * If we haven't had food for a while, let it be food. */
  int r;
  if (Game::levels_without_food > 3)
    r = 2;
  else
    r = static_cast<int>(pick_one(things, things.size()));

  Item* cur = nullptr;
  switch (r)
  {
    case 0: cur = potion_create(-1); break;
    case 1: cur = scroll_create(-1); break;
    case 2: cur = new_food(-1); break;
    case 3: cur = weapon_create(-1, true); break;
    case 4: cur = armor_create(-1, true); break;
    case 5: cur = ring_create(-1, true); break;
    case 6: cur = wand_create(-1); break;
    default:
      io_msg("Picked a bad kind of object (this should not happen)");
      io_wait_for_key(KEY_SPACE);
      break;
  }

  if (cur == nullptr) {
    throw runtime_error("curr is null");
  }
  if (cur->o_damage.sides < 0 || cur->o_damage.dices < 0) {
    throw runtime_error("cur->damage is negative");
  }
  if (cur->o_hurldmg.sides < 0 || cur->o_hurldmg.dices < 0) {
    throw runtime_error("cur->hurldmg is negative");
  }
  return cur;
}

size_t
pick_one(vector<obj_info>& start, size_t nitems)
{
  for (size_t rand = static_cast<size_t>(os_rand_range(100)), i = 0; i < nitems; ++i)
    if (rand < start.at(i).oi_prob)
      return i;
    else
      rand -= start.at(i).oi_prob;

  /* The functions should have returned by now */
  throw range_error("Probability was not in range");
}

/* list what the player has discovered of this type */
static void
discovered_by_type(char type, vector<obj_info>& info, size_t max_items)
{
  WINDOW *printscr = dupwin(stdscr);

  Coordinate orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  Item printable_object;
  memset(&printable_object, 0, sizeof(printable_object));
  printable_object.o_type = type;
  printable_object.o_flags = 0;
  printable_object.o_count = 1;

  int items_found = 0;
  for (size_t i = 0; i < max_items; ++i)
    if (info.at(i).oi_know || !info.at(i).oi_guess.empty())
    {
      printable_object.o_which = static_cast<int>(i);
      mvwprintw(printscr, ++items_found, 1,
                "%s", inv_name(&printable_object, false).c_str());
    }

  if (items_found == 0)
  {
    string type_as_str = nullptr;
    switch (type)
    {
      case POTION: type_as_str = "potion"; break;
      case SCROLL: type_as_str = "scroll"; break;
      case RING:   type_as_str = "ring"; break;
      case STICK:  type_as_str = "stick"; break;
    }
    mvwprintw(printscr, 1, 1, "No known %s", type_as_str.c_str());
  }

  move(orig_pos.y, orig_pos.x);
  wrefresh(printscr);
  delwin(printscr);
}

void
discovered(void)
{
  io_msg("for what type of objects do you want a list? (%c%c%c%c) ",
      POTION, SCROLL, RING, STICK);
  while (true)
  {
    char ch = io_readchar(true);
    touchwin(stdscr);
    refresh();
    switch (ch)
    {
      case POTION: discovered_by_type(ch, potion_info, NPOTIONS); break;
      case SCROLL: discovered_by_type(ch, scroll_info, NSCROLLS); break;
      case RING: discovered_by_type(ch, ring_info, NRINGS); break;
      case STICK: discovered_by_type(ch, wands_info, MAXSTICKS); break;
      default: io_msg_clear(); return;
    }
  }

  touchwin(stdscr);
  io_msg_clear();
}

