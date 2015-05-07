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
#include "rogue.h"

void
pr_spec(char ch)
{
  WINDOW *printscr = dupwin(stdscr);
  int lastprob = 0;
  coord orig_pos;
  int i;
  int max;
  void *ptr;

  getyx(stdscr, orig_pos.y, orig_pos.x);

  switch (ch)
  {
    case POTION: ptr = pot_info;          max = NPOTIONS;  break;
    case SCROLL: ptr = scr_info;          max = NSCROLLS;  break;
    case RING:   ptr = ring_info;         max = NRINGS;    break;
    case STICK:  ptr = __wands_ptr();     max = MAXSTICKS; break;
    case ARMOR:  ptr = __armors_ptr();    max = NARMORS;   break;
    case WEAPON: ptr = weap_info;         max = MAXWEAPONS;break;
    default:     ptr = NULL;              max = 0;         break;
  }

  for (i = 0, ch = '0'; i < max; ++i)
  {
    const char *name;
    int prob;
    wmove(printscr, i + 1, 1);

    if (ptr == __armors_ptr())
    {
      name = ((struct armor_info_t *)ptr)[i].name;
      prob = ((struct armor_info_t *)ptr)[i].prob - lastprob;
      lastprob = ((struct armor_info_t *)ptr)[i].prob;
    }
    else
    {
      name = ((struct obj_info *)ptr)[i].oi_name;
      prob = ((struct obj_info *)ptr)[i].oi_prob - lastprob;
      lastprob = ((struct obj_info *)ptr)[i].oi_prob;
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
  char index_to_char[] = {
    POTION, SCROLL, FOOD,
    WEAPON, ARMOR, RING,
    STICK
  };
  unsigned num_items = 0;
  unsigned i;
  unsigned end = NUMTHINGS;
  WINDOW *tmp = dupwin(stdscr);
  coord orig_pos;

  getyx(stdscr, orig_pos.y, orig_pos.x);

  for (i = 0; i < end; ++i)
  {
    wmove(tmp, ++num_items, 1);
    wprintw(tmp, "%c) %s", index_to_char[i], things[i].oi_name);
  }

  wmove(stdscr, orig_pos.y, orig_pos.x);
  wrefresh(tmp);
  delwin(tmp);
}

int
pr_list(void)
{
  int ch = ~KEY_ESCAPE;

  msg("for what type of object do you want a list? ");
  print_things();

  ch = readchar();
  touchwin(stdscr);
  refresh();

  pr_spec(ch);
  msg("");
  msg("--Press any key to continue--");
  readchar();

  msg("");
  touchwin(stdscr);
  return 0;
}

void
create_obj(void)
{
  THING *obj = NULL;
  char ch;
  int type;
  int which;

  msg("type of item: ");
  type = readchar();
  mpos = 0;

  if (!(type == WEAPON || type == ARMOR || type == RING || type == STICK
      || type == GOLD || type == POTION || type == SCROLL))
  {
    msg("Bad pick");
    return;
  }

  msg("which %c do you want? (0-f)", type);
  which = (isdigit((ch = readchar())) ? ch - '0' : ch - 'a' + 10);
  mpos = 0;

  if (type == STICK)
    obj = wand_create(which);
  else
  {
    obj = new_item();
    obj->o_type = type;
    obj->o_which = which;
    obj->o_group = 0;
    obj->o_count = 1;
  }

  switch (type)
  {
    case WEAPON: case ARMOR:
      {
        char bless;
        msg("blessing? (+,-,n)");
        bless = readchar();
        mpos = 0;
        if (bless == '-')
          obj->o_flags |= ISCURSED;
        if (obj->o_type == WEAPON)
        {
          init_weapon(obj, obj->o_which);
          if (bless == '-')
            obj->o_hplus -= rnd(3)+1;
          if (bless == '+')
            obj->o_hplus += rnd(3)+1;
        }
        else
        {
          obj->o_arm = armor_ac(obj->o_which);
          if (bless == '-')
            obj->o_arm += rnd(3)+1;
          if (bless == '+')
            obj->o_arm -= rnd(3)+1;
        }
      }
      break;

    case RING:
      {
        char bless;
        switch (obj->o_which)
        {
          case R_PROTECT:
          case R_ADDSTR:
          case R_ADDHIT:
          case R_ADDDAM:
            msg("blessing? (+,-,n)");
            bless = readchar();
            mpos = 0;
            if (bless == '-')
              obj->o_flags |= ISCURSED;
            obj->o_arm = (bless == '-' ? -1 : rnd(2) + 1);
            break;
          case R_AGGR: case R_TELEPORT:
            obj->o_flags |= ISCURSED;
            break;
        }
      }
      break;


    case GOLD:
      {
        char buf[MAXSTR] = { '\0' };
        msg("how much?");
        if (readstr(buf) == 0)
          obj->o_goldval = (short) atoi(buf);
      }
      break;
  }

  assert(obj != NULL);
  pack_add(obj, false);
}

void
show_map(void)
{
  int y;
  int x;

  wclear(hw);
  for (y = 1; y < NUMLINES - 1; y++)
    for (x = 0; x < NUMCOLS; x++)
    {
      int real = flat(y, x);
      if (!(real & F_REAL))
        wstandout(hw);
      wmove(hw, y, x);
      waddcch(hw, chat(y, x));
      if (!real)
        wstandend(hw);
    }
  show_win("---More (level map)---");
}

void
wizard_levels_and_gear(void)
{
  int i;
  THING *obj;

  for (i = 0; i < 9; i++)
    player_raise_level();

  /* Give him a sword (+1,+1) */
  if (pack_equipped_item(EQUIPMENT_RHAND) == NULL)
  {
    obj = new_item();
    init_weapon(obj, TWOSWORD);
    obj->o_hplus = 1;
    obj->o_dplus = 1;
    pack_equip_item(obj);
  }

  /* And his suit of armor */
  if (pack_equipped_item(EQUIPMENT_ARMOR) == NULL)
  {
    obj = new_item();
    obj->o_type = ARMOR;
    obj->o_which = PLATE_MAIL;
    obj->o_arm = -5;
    obj->o_flags |= ISKNOW;
    obj->o_count = 1;
    obj->o_group = 0;
    pack_equip_item(obj);
  }
}
