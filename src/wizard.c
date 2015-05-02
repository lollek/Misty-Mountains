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
    case STICK:  ptr = ws_info;           max = MAXSTICKS; break;
    case ARMOR:  ptr = __armors_ptr();    max = NARMORS;   break;
    case WEAPON: ptr = weap_info;         max = MAXWEAPONS;break;
    default:     ptr = NULL;              max = 0;         break;
  }

  for (i = 0, ch = '0'; i < max; ++i)
  {
    const char *name;
    int prob;
    ch = ch == '9' ? 'a' : (ch + 1);
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
  }
  wmove(stdscr, orig_pos.y, orig_pos.x);
  wrefresh(printscr);
  delwin(printscr);
}

int
pr_list(void)
{
  int ch = ~KEY_ESCAPE;

  if (!terse)
    addmsg("for ");
  addmsg("what type");
  if (!terse)
    addmsg(" of object do you want a list");
  msg("? ");

  while (ch != KEY_ESCAPE)
  {
    ch = readchar();
    touchwin(stdscr);
    refresh();
    pr_spec(ch);
  }
  touchwin(stdscr);
  msg("");
  return 0;
}

void
create_obj(void)
{
    THING *obj;
    char ch, bless;

    obj = new_item();
    msg("type of item: ");
    obj->o_type = readchar();
    mpos = 0;
    msg("which %c do you want? (0-f)", obj->o_type);
    obj->o_which = (isdigit((ch = readchar())) ? ch - '0' : ch - 'a' + 10);
    obj->o_group = 0;
    obj->o_count = 1;
    mpos = 0;
    if (obj->o_type == WEAPON || obj->o_type == ARMOR)
    {
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
    else if (obj->o_type == RING)
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
    else if (obj->o_type == STICK)
	fix_stick(obj);
    else if (obj->o_type == GOLD)
    {
	char buf[MAXSTR] = { '\0' };
	msg("how much?");
	if (readstr(buf) == 0)
	    obj->o_goldval = (short) atoi(buf);
    }
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
