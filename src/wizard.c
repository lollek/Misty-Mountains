/*
 * Special wizard commands (some of which are also non-wizard commands
 * under strange circumstances)
 *
 * @(#)wizard.c	4.30 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "potions.h"
#include "scrolls.h"
#include "command.h"
#include "options.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "list.h"
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

int wizard = false;

static void
pr_spec(char type)
{
  WINDOW* printscr = dupwin(stdscr);

  coord orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  void* ptr;
  int max;
  switch (type)
  {
    case POTION: ptr = potion_info;          max = NPOTIONS;  break;
    case SCROLL: ptr = scroll_info;          max = NSCROLLS;  break;
    case RING:   ptr = ring_info;         max = NRINGS;    break;
    case STICK:  ptr = __wands_ptr();     max = MAXSTICKS; break;
    case ARMOR:  ptr = NULL;              max = NARMORS;   break;
    case WEAPON: ptr = weapon_info;         max = MAXWEAPONS;break;
    default:     ptr = NULL;              max = 0;         break;
  }

  char ch = '0';
  for (int i = 0; i < max; ++i)
  {
    char const* name;
    int prob;
    wmove(printscr, i + 1, 1);

    if (type == ARMOR)
    {
      name = armor_name((enum armor_t)i);
      prob = armor_probability((enum armor_t)i);
    }
    else
    {
      name = ((struct obj_info *)ptr)[i].oi_name;
      prob = ((struct obj_info *)ptr)[i].oi_prob;
    }
    wprintw(printscr, "%c: %s (%d%%)", ch, name, prob);
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

  coord orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  for (int i = 0; i < NUMTHINGS; ++i)
  {
    wmove(tmp, i + 1, 1);
    wprintw(tmp, "%c %s", index_to_char[i], things[i].oi_name);
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

  pr_spec((char)ch);
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

  THING* obj = NULL;
  switch (type)
  {
    case TRAP:
      {
        if (which < 0 || which >= NTRAPS)
          io_msg("Bad trap id");
        else
        {
          coord *player_pos = player_get_pos();
          char flags = level_get_flags(player_pos->y, player_pos->x);
          flags &= ~F_REAL;
          flags |= which;
          level_set_flags(player_pos->y, player_pos->x, flags);
        }
        return;
      }
    case STICK: obj = wand_create(which); break;
    case SCROLL: obj = scroll_create(which); break;
    case POTION: obj = potion_create(which); break;
    case FOOD: obj = new_food(which); break;
    case WEAPON:
      {
        obj = weapon_create(which, false);

        io_msg("blessing? (+,-,n)");
        char bless = io_readchar(true);
        io_msg_clear();

        if (bless == '-')
        {
          obj->o.o_flags |= ISCURSED;
          obj->o.o_hplus -= os_rand_range(3) + 1;
        }
        else if (bless == '+')
          obj->o.o_hplus += os_rand_range(3) + 1;
      }
      break;

    case ARMOR:
      {
        obj = armor_create(-1, false);

        io_msg("blessing? (+,-,n)");
        char bless = io_readchar(true);
        io_msg_clear();
        if (bless == '-')
        {
          obj->o.o_flags |= ISCURSED;
          obj->o.o_arm += os_rand_range(3) + 1;
        }
        else if (bless == '+')
          obj->o.o_arm -= os_rand_range(3) + 1;
      }
      break;

    case RING:
      {
        obj = ring_create(which, false);
        switch (obj->o.o_which)
        {
          case R_PROTECT: case R_ADDSTR: case R_ADDHIT: case R_ADDDAM:
            io_msg("blessing? (+,-,n)");
            char bless = io_readchar(true);
            io_msg_clear();
            if (bless == '-')
              obj->o.o_flags |= ISCURSED;
            obj->o.o_arm = (bless == '-' ? -1 : os_rand_range(2) + 1);
            break;
        }
      }
      break;

    case GOLD:
      {
        char buf[MAXSTR] = { '\0' };
        io_msg("how much?");
        if (io_readstr(buf) == 0)
          obj->o.o_goldval = (short) atoi(buf);
      }
      break;

    default:
      io_debug("Not implemented: %c(%d)\r\n", which, which);
      assert(0);
      break;
  }

  assert(obj != NULL);
  pack_add(obj, false);
}

void
wizard_show_map(void)
{
  wclear(hw);
  for (int y = 1; y < NUMLINES - 1; y++)
    for (int x = 0; x < NUMCOLS; x++)
    {
      int real = level_get_flags(y, x);
      if (!(real & F_REAL))
        wstandout(hw);
      wmove(hw, y, x);
      waddcch(hw, level_get_ch(y, x));
      if (!real)
        wstandend(hw);
    }
  show_win("---More (level map)---");
}

void
wizard_levels_and_gear(void)
{
  for (int i = 0; i < 9; i++)
    player_raise_level();

  /* Give him a sword (+1,+1) */
  if (pack_equipped_item(EQUIPMENT_RHAND) == NULL)
  {
    THING* obj = weapon_create(TWOSWORD, false);
    obj->o.o_hplus = 1;
    obj->o.o_dplus = 1;
    pack_equip_item(obj);
  }

  /* And his suit of armor */
  if (pack_equipped_item(EQUIPMENT_ARMOR) == NULL)
  {
    THING* obj = armor_create(PLATE_MAIL, false);
    obj->o.o_arm = -5;
    obj->o.o_flags |= ISKNOW;
    obj->o.o_count = 1;
    pack_equip_item(obj);
  }
}
