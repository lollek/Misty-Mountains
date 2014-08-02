/*
 * Routines to deal with the pack
 *
 * @(#)pack.c	4.40 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <string.h>

#include "rogue.h"
#include "status_effects.h"
#include "scrolls.h"
#include "io.h"

static char pack_char();
static void move_msg(THING *obj);
static void money(int value);
static char floor_ch();
static void remove_from_floor(THING *obj);

/** add_pack:
 * Pick up an object and add it to the pack.  If the argument is
 * non-null use it as the linked_list pointer instead of gettting
 * it off the ground. */

void
add_pack(THING *obj, bool silent)
{
    THING *op;
    bool from_floor = false;

    /* Either obj in an item or we try to take something from the floor */
    if (obj == NULL)
    {
	if ((obj = find_obj(hero.y, hero.x)) == NULL)
	    return;
	from_floor = true;
    }

    /* Check for and deal with scare monster scrolls */
    if (obj->o_type == SCROLL && obj->o_which == S_SCARE &&
        obj->o_flags & ISFOUND)
    {
	detach(lvl_obj, obj);
	mvaddcch(hero.y, hero.x, floor_ch());
	chat(hero.y, hero.x) = (proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
	discard(obj);
	msg("the scroll turns to dust as you pick it up");
	return;
    }

    if (items_in_pack() == PACKSIZE)
    {
	msg(terse
	    ? "no room"
	    : "there's no room in your pack");
	if (from_floor)
	    move_msg(obj);
	return;
    }

    if (pack == NULL)
    {
	if (from_floor)
	    remove_from_floor(obj);
	attach(pack, obj);
	obj->o_packch = pack_char();
    }
    else
    {
	THING *lp = NULL;
	for (op = pack; op != NULL; op = next(op))
	{
	    if (op->o_type != obj->o_type)
		lp = op;
	    else
	    {
		while (op->o_type == obj->o_type && op->o_which != obj->o_which)
		{
		    lp = op;
		    if (next(op) == NULL)
			break;
		    else
			op = next(op);
		}
		if (op->o_type == obj->o_type && op->o_which == obj->o_which)
		{
		    if (op->o_type == POTION || op->o_type == SCROLL ||
                        obj->o_type == FOOD)
		    {
			if (from_floor)
			    remove_from_floor(obj);
			op->o_count++;
			discard(obj);
			obj = op;
			lp = NULL;
			break;
		    }
		    else if (obj->o_group)
		    {
			lp = op;
			while (op->o_type == obj->o_type
			    && op->o_which == obj->o_which
			    && op->o_group != obj->o_group)
			{
			    lp = op;
			    if (next(op) == NULL)
				break;
			    else
				op = next(op);
			}
			if (op->o_type == obj->o_type
			    && op->o_which == obj->o_which
			    && op->o_group == obj->o_group)
			{
				op->o_count += obj->o_count;
				if (from_floor)
				    remove_from_floor(obj);
				discard(obj);
				obj = op;
				lp = NULL;
				break;
			}
		    }
		    else
			lp = op;
		}
		break;
	    }
	}

	if (lp != NULL)
	{
	    if (from_floor)
		remove_from_floor(obj);
	    obj->o_packch = pack_char();
	    next(obj) = next(lp);
	    prev(obj) = lp;
	    if (next(lp) != NULL)
		prev(next(lp)) = obj;
	    next(lp) = obj;
	}
    }

    obj->o_flags |= ISFOUND;

    /*
     * If this was the object of something's desire, that monster will
     * get mad and run at the hero.
     */
    for (op = mlist; op != NULL; op = next(op))
	if (op->t_dest == &obj->o_pos)
	    op->t_dest = &hero;

    /* Notify the user */
    if (!silent)
    {
	if (!terse)
	    addmsg("you now have ");
	msg("%s (%c)", inv_name(obj, !terse), obj->o_packch);
    }
}

/*
 * leave_pack:
 *	take an item out of the pack
 */
THING *
leave_pack(THING *obj, bool newobj, bool all)
{
    THING *nobj;

    nobj = obj;
    if (obj->o_count > 1 && !all)
    {
	last_pick = obj;
	obj->o_count--;
	if (newobj)
	{
	    nobj = new_item();
	    *nobj = *obj;
	    next(nobj) = NULL;
	    prev(nobj) = NULL;
	    nobj->o_count = 1;
	}
    }
    else
    {
	last_pick = NULL;
	pack_used[obj->o_packch - 'a'] = false;
	detach(pack, obj);
    }
    return nobj;
}

/*
 * pack_char:
 *	Return the next unused pack character.
 */
static char
pack_char(void)
{
    bool *bp;

    for (bp = pack_used; *bp; bp++)
	continue;
    *bp = true;
    return (char)((int)(bp - pack_used) + 'a');
}

/*
 * pick_up:
 *	Add something to characters pack.
 */

/* TODO: Maybe move this to command.c? */
void
pick_up(char ch)
{
    THING *obj;

    if (is_levitating(&player))
	return;

    obj = find_obj(hero.y, hero.x);
    if (move_on)
	move_msg(obj);
    else
	switch (ch)
	{
	    case GOLD:
		if (obj == NULL)
		    return;
		money(obj->o_goldval);
		detach(lvl_obj, obj);
		discard(obj);
		proom->r_goldval = 0;
		break;
	    default:
	    if (wizard)
		msg("Where did you pick a '%s' up???", unctrl(ch));
	    case ARMOR:
	    case POTION:
	    case FOOD:
	    case WEAPON:
	    case SCROLL:	
	    case AMULET:
	    case RING:
	    case STICK:
		add_pack((THING *) NULL, false);
		break;
	}
}

/*
 * move_msg:
 *	Print out the message if you are just moving onto an object
 */

static void
move_msg(THING *obj)
{
    if (!terse)
	addmsg("you ");
    msg("moved onto %s", inv_name(obj, true));
}

/*
 * picky_inven:
 *	Allow player to inventory a single item
 */

/* TODO: Maybe move this to command.c? */
void
picky_inven(void)
{
    THING *obj;
    char mch;

    if (pack == NULL)
	msg("you aren't carrying anything");
    else if (next(pack) == NULL)
	msg("a) %s", inv_name(pack, false));
    else
    {
	msg(terse ? "item: " : "which item do you wish to inventory: ");
	mpos = 0;
	if ((mch = readchar()) == KEY_ESCAPE)
	{
	    msg("");
	    return;
	}
	for (obj = pack; obj != NULL; obj = next(obj))
	    if (mch == obj->o_packch)
	    {
		msg("%c) %s", mch, inv_name(obj, false));
		return;
	    }
	msg("'%s' not in pack", unctrl(mch));
    }
}

/** get_item:
 * Pick something out of a pack for a purpose */
THING *
get_item(const char *purpose, int type)
{
  if (again)
    if (last_pick)
      return last_pick;
    else
    {
      msg("you ran out");
      return NULL;
    }

  /* Make sure theres an item of the type */
  else if (items_in_pack_of_type(type) == 0)
  {
    msg("You have no item to %s", purpose);
    return NULL;
  }

  for (;;)
  {
    THING *obj;
    char ch;

    if (!terse)
      addmsg("which object do you want to ");
    addmsg(purpose);
    if (terse)
      addmsg(" what");
    msg("? (* for list): ");
    ch = readchar();
    mpos = 0;

    /* Give the poor player a chance to abort the command */
    if (ch == KEY_ESCAPE)
    {
      reset_last();
      after = false;
      clear_inventory(); /* Hide inventory in case we've showed it */
      msg("");
      return NULL;
    }

    /* normal case: person types one char */
    n_objs = 1;
    if (ch == '*')
    {
      mpos = 0;
      print_inventory(type);
      continue;
    }

    for (obj = pack; obj != NULL; obj = next(obj))
      if (obj->o_packch == ch)
        break;
    if (obj == NULL)
    {
      msg("'%s' is not a valid item",unctrl(ch));
      continue;
    }
    else
    {
      clear_inventory(); /* Hide inventory in case we've showed it */
      return obj;
    }
  }
}

/** money:
 * Add or subtract gold from the pack */
/* TODO: Maybe inline this function? */
static void
money(int value)
{
  purse += value;
  mvaddcch(hero.y, hero.x, floor_ch());
  chat(hero.y, hero.x) = (proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
  if (value > 0)
  {
    if (!terse)
      addmsg("you found ");
    msg("%d gold pieces", value);
  }
}

/** floor_ch:
 * Return the appropriate floor character for her room */
static char
floor_ch(void)
{
  if (proom->r_flags & ISGONE)
    return PASSAGE;
  return show_floor() ? FLOOR : SHADOW;
}

/** floor_at:
 * Return the character at hero's position, taking see_floor
 * into account */
char
floor_at(void)
{
  char ch = chat(hero.y, hero.x);
  return ch == FLOOR ? floor_ch() : ch;
}

/** reset_last:
 * Reset the last command when the current one is aborted */
void
reset_last(void)
{
  last_comm = l_last_comm;
  last_dir = l_last_dir;
  last_pick = l_last_pick;
}

/** remove_from_floor
 * Removes one item from the floor */
static void
remove_from_floor(THING *obj)
{
  detach(lvl_obj, obj);
  mvaddcch(hero.y, hero.x, floor_ch());
  chat(hero.y, hero.x) = (proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
}

/** items_in_pack
 * Counts how many items she is carrying */
unsigned
items_in_pack(void)
{
  return items_in_pack_of_type(0);
}

/** items_in_pack
 * Counts how many items she is carrying of a certain type */
unsigned
items_in_pack_of_type(int type)
{
  unsigned num = 0;
  THING *list;

  for (list = pack; list != NULL; list = next(list))
    if (!type || type == list->o_type ||
        (type == CALLABLE && (list->o_type != FOOD && list->o_type != AMULET))||
        (type == R_OR_S && (list->o_type == RING || list->o_type == STICK)))
      ++num;
  return num;
}

bool
player_has_amulet(void)
{
  THING *ptr;

  for (ptr = pack; ptr != NULL; ptr = next(ptr))
    if (ptr->o_type == AMULET)
      return true;
  return false;
}

bool
print_inventory(int type)
{
  unsigned num_items = 0;
  THING *list;
  WINDOW *invscr = dupwin(stdscr);
  coord orig_pos;

  getyx(stdscr, orig_pos.y, orig_pos.x);

  /* Print out all items */
  for (list = pack; list != NULL; list = next(list))
  {
    if (!type || type == list->o_type ||
        (type == CALLABLE && (list->o_type != FOOD && list->o_type != AMULET))||
        (type == R_OR_S && (list->o_type == RING || list->o_type == STICK)))
    {
      /* Print out the item and move to next row */
      wmove(invscr, ++num_items, 1);
      wprintw(invscr, "%c) %s", list->o_packch, inv_name(list, false));
    }
  }

  wmove(stdscr, orig_pos.y, orig_pos.x);
  wrefresh(invscr);
  delwin(invscr);
  return num_items != 0;
}

void
clear_inventory(void)
{
  touchwin(stdscr);
}
