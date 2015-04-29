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
#include "rogue.h"

static char *type_name(int type);

/** pr_spec:
 * Print specific list of possible items to choose from */
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
    case POTION: ptr = pot_info;          max = NPOTIONS;
    when SCROLL: ptr = scr_info;          max = NSCROLLS;
    when RING:   ptr = ring_info;         max = NRINGS;
    when STICK:  ptr = ws_info;           max = MAXSTICKS;
    when ARMOR:  ptr = __armors_ptr();    max = NARMORS;
    when WEAPON: ptr = weap_info;         max = MAXWEAPONS;
    otherwise:   ptr = NULL;              max = 0;
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

/** pr_list:
 * List possible potions, scrolls, etc. for wizard. */
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



/*
 * whatis:
 *	What a certin object is
 */

void
whatis(int type)
{
    THING *obj;

    if (pack_is_empty())
    {
	msg("you don't have anything in your pack to identify");
	return;
    }

    for (;;)
    {
	obj = pack_get_item("identify", type);
	if (obj == NULL)
	    return;
	else if (type && obj->o_type != type && !(type == R_OR_S &&
		(obj->o_type == RING || obj->o_type == STICK)))
	    msg("you must identify a %s", type_name(type));
	else
	    break;
    }

    switch (obj->o_type)
    {
	case SCROLL: set_know(obj, scr_info);
	when POTION: set_know(obj, pot_info);
	when STICK: set_know(obj, ws_info);
	when WEAPON: case ARMOR: obj->o_flags |= ISKNOW;
	when RING: set_know(obj, ring_info);
    }
    msg(inv_name(obj, false));
}

/*
 * set_know:
 *	Set things up when we really know what a thing is
 */

void
set_know(THING *obj, struct obj_info *info)
{
    char **guess;

    info[obj->o_which].oi_know = true;
    obj->o_flags |= ISKNOW;
    guess = &info[obj->o_which].oi_guess;
    if (*guess)
    {
	free(*guess);
	*guess = NULL;
    }
}

/*
 * type_name:
 *	Return a pointer to the name of the type
 */
char *
type_name(int type)
{
    struct h_list *hp;
    static struct h_list tlist[] = {
	{POTION, "potion",		false},
	{SCROLL, "scroll",		false},
	{FOOD,	 "food",		false},
	{R_OR_S, "ring, wand or staff",	false},
	{RING,	 "ring",		false},
	{STICK,	 "wand or staff",	false},
	{WEAPON, "weapon",		false},
	{ARMOR,	 "suit of armor",	false},
    };

    for (hp = tlist; hp->h_ch; hp++)
	if (type == hp->h_ch)
	    return hp->h_desc;
    /* NOTREACHED */
    return(0);
}

/*
 * create_obj:
 *	wizard command for getting anything he wants
 */

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
	    when R_AGGR:
	    case R_TELEPORT:
		obj->o_flags |= ISCURSED;
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

/*
 * show_map:
 *	Print out the map for the wizard
 */

void
show_map(void)
{
    int y, x, real;

    wclear(hw);
    for (y = 1; y < NUMLINES - 1; y++)
	for (x = 0; x < NUMCOLS; x++)
	{
	    real = flat(y, x);
	    if (!(real & F_REAL))
		wstandout(hw);
	    wmove(hw, y, x);
	    waddcch(hw, chat(y, x));
	    if (!real)
		wstandend(hw);
	}
    show_win("---More (level map)---");
}
