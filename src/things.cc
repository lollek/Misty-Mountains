/*
 * Contains functions for dealing with things like potions, scrolls,
 * and other items.
 *
 * @(#)things.c	4.53 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#include <vector>
#include <string>

using namespace std;

#include "Coordinate.h"
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

char*
inv_name(char* buf, item const* item, bool drop)
{
  buf[MAXSTR -1] = '\0';
  switch (item->o_type)
  {
    case POTION: potion_description(item, buf); break;
    case RING:   ring_description(item, buf); break;
    case STICK:  wand_description(item, buf); break;
    case SCROLL: scroll_description(item, buf); break;
    case WEAPON: case AMMO: weapon_description(item, buf); break;
    case ARMOR:  armor_description(item, buf); break;
    case FOOD:
    {
      char const* obj_type = item->o_which == 1 ? "fruit" : "food ration";
      if (item->o_count == 1)
        sprintf(buf, "A %s", obj_type);
      else
        sprintf(buf, "%d %ss", item->o_count, obj_type);
    }
    break;
    case AMULET: strcpy(buf, "The Amulet of Yendor"); break;
    case GOLD:   sprintf(buf, "%d Gold pieces", item->o_goldval); break;
    default:
      io_msg("You feel a disturbance in the force");
      sprintf(buf, "Something bizarre %s", unctrl(static_cast<chtype>(item->o_type)));
      break;
  }

  buf[0] = static_cast<char>(drop ? tolower(buf[0]) : toupper(buf[0]));
  assert (buf[MAXSTR -1] == '\0');
  return buf;
}

item*
new_food(int which)
{
  /* Reset levels-without-food counter */
  levels_without_food = 0;

  item* cur = new item();
  cur->o_count = 1;
  cur->o_type = FOOD;
  switch (which)
  {
    case 0: case 1: cur->o_which = which; break;
    default: cur->o_which = os_rand_range(10) ? 0 : 1; break;
  }
  return cur;
}

item*
new_amulet(void)
{
  item* obj       = new item();
  obj->o_damage  = {1, 2};
  obj->o_hurldmg = {1, 2};
  obj->o_type    = AMULET;

  return obj;
}

item*
new_thing(void)
{
  /* Decide what kind of object it will be
   * If we haven't had food for a while, let it be food. */
  int r;
  if (levels_without_food > 3)
    r = 2;
  else
    r = static_cast<int>(pick_one(things, things.size()));

  item* cur = nullptr;
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

  assert(cur != nullptr);
  assert(cur->o_damage.sides >= 0 && cur->o_damage.dices >= 0);
  assert(cur->o_hurldmg.sides >= 0 && cur->o_hurldmg.dices >= 0);
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

  item printable_object;
  memset(&printable_object, 0, sizeof(printable_object));
  printable_object.o_type = type;
  printable_object.o_flags = 0;
  printable_object.o_count = 1;

  int items_found = 0;
  for (size_t i = 0; i < max_items; ++i)
    if (info.at(i).oi_know || !info.at(i).oi_guess.empty())
    {
      char buf[MAXSTR];
      printable_object.o_which = static_cast<int>(i);
      mvwprintw(printscr, ++items_found, 1,
                "%s", inv_name(buf, &printable_object, false));
    }

  if (items_found == 0)
  {
    char const* type_as_str = nullptr;
    switch (type)
    {
      case POTION: type_as_str = "potion"; break;
      case SCROLL: type_as_str = "scroll"; break;
      case RING:   type_as_str = "ring"; break;
      case STICK:  type_as_str = "stick"; break;
    }
    mvwprintw(printscr, 1, 1, "No known %s", type_as_str);
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

