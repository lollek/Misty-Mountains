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

#include "rogue.h"
#include "potions.h"
#include "status_effects.h"
#include "scrolls.h"

static bool dropcheck(THING *obj);
static int pick_one(struct obj_info *info, int nitems);
static void nameit(THING *obj, const char *type, const char *which,
                   struct obj_info *op, char *(*prfunc)(THING *));
static char *nullstr(THING *ignored);
static void discovered_by_type(char type, struct obj_info *info,
                               int max_items);

/** inv_name:
 * Return the name of something as it would appear in an inventory. */
char *
inv_name(THING *obj, bool drop)
{
  char *pb = prbuf;
  int which = obj->o_which;

  switch (obj->o_type)
  {
    case POTION:
      nameit(obj, "potion", p_colors[which], &pot_info[which], nullstr);
    when RING:
      nameit(obj, "ring", r_stones[which], &ring_info[which], ring_num);
    when STICK:
      nameit(obj, ws_type[which], ws_made[which], &ws_info[which], charge_str);

    when SCROLL:
    {
      struct obj_info *op = &scr_info[which];

      if (obj->o_count == 1)
        pb += strlen(strcpy(pb, "A scroll "));
      else
        pb += sprintf(pb, "%d scrolls ", obj->o_count);

      if (op->oi_know)
        pb += sprintf(pb, "of %s", op->oi_name);
      else if (op->oi_guess)
        pb += sprintf(pb, "called %s", op->oi_guess);
      else
        pb += sprintf(pb, "titled '%s'", s_names[which]);
    }

    when FOOD:
    {
      const char *food_type = obj->o_which == 1 ? "fruit" : "ration of food";

      if (obj->o_count == 1)
        pb += sprintf(pb, "A %s", food_type);
      else
        pb += sprintf(pb, "%d %ss", obj->o_count, food_type);
    }

    when WEAPON:
    {
      const char *sp = weap_info[which].oi_name;

      if (obj->o_count == 1)
        pb += sprintf(pb, "A%s %s", vowelstr(sp), sp);
      else
        pb += sprintf(pb, "%d %ss", obj->o_count, sp);

      if (which != ARROW)
      {
        /* TODO: Maybe we can rename NumxNum -> NumdNum everywhere? */
        char damage[4] = { '\0' };

        sprintf(damage, "%s", which == BOW ? obj->o_hurldmg : obj->o_damage);
        damage[1] = 'd';
        pb += sprintf(pb, " (%s)", damage);
      }

      if (obj->o_flags & ISKNOW)
        pb += sprintf(pb, " (%s)", num(obj->o_hplus,obj->o_dplus,WEAPON));

      if (obj->o_label != NULL)
        pb += sprintf(pb, " called %s", obj->o_label);
    }

    when ARMOR:
    {
      const char *sp = arm_info[which].oi_name;

      pb += sprintf(pb, "A%s %s", vowelstr(sp), sp);

      if (obj->o_flags & ISKNOW)
      {
        int bonus_ac = atoi(num(a_class[which] - obj->o_arm, 0, ARMOR) + 1);
        int base_ac = 10 - obj->o_arm - bonus_ac;
        pb += sprintf(pb, " [%d,+%d]", base_ac, bonus_ac);
      }

      if (obj->o_label != NULL)
        pb += sprintf(pb, " called %s", obj->o_label);
    }

    when AMULET:
      pb += strlen(strcpy(pb, "The Amulet of Yendor"));
    when GOLD:
      pb += sprintf(pb, "%d Gold pieces", obj->o_goldval);
    otherwise:
      msg("You feel a disturbance in the force");
      pb += sprintf(pb, "Something bizarre %s", unctrl(obj->o_type));
  }

  if (inv_describe)
  {
    if (obj == cur_armor)
      strcat(pb, " (being worn)");
    else if (obj == cur_weapon)
      strcat(pb, " (weapon in hand)");
    else if (obj == cur_ring[LEFT])
      strcat(pb, " (on left hand)");
    else if (obj == cur_ring[RIGHT])
      strcat(pb, " (on right hand)");
  }

  prbuf[0] = drop ? tolower(prbuf[0]) : toupper(prbuf[0]);
  prbuf[MAXSTR-1] = '\0';
  return prbuf;
}

/** drop:
 * Put something down */
/* TODO: Maybe move this to command.c */
bool
drop(void)
{
    char ch = chat(hero.y, hero.x);
    THING *obj;

    if (ch != FLOOR && ch != PASSAGE)
    {
	after = false;
	msg("there is something there already");
	return false;
    }
    if ((obj = get_item("drop", 0)) == NULL)
	return false;
    if (!dropcheck(obj))
	return false;
    obj = leave_pack(obj, true, !(obj->o_type == POTION ||
          obj->o_type == SCROLL || obj->o_type == FOOD));

    /* Link it into the level object list */
    attach(lvl_obj, obj);
    chat(hero.y, hero.x) = (char) obj->o_type;
    flat(hero.y, hero.x) |= F_DROPPED;
    obj->o_pos = hero;
    msg("dropped %s", inv_name(obj, true));
    return true;
}

/** dropcheck:
 * Check if we can remove it before dropping */
static bool
dropcheck(THING *obj)
{
    if (obj == NULL)
	return true;
    if (obj == cur_weapon)
    {
	if (obj->o_flags & ISCURSED)
	    msg("you can't.  It appears to be cursed");
	else
	    cur_weapon = NULL;
	return cur_weapon == NULL;
    }
    else if (obj == cur_armor)
    {
	take_off();
	return cur_armor == NULL;
    }
    else if (obj == cur_ring[LEFT])
    {
	ring_off();
	return cur_ring[LEFT] == NULL;
    }
    else if (obj == cur_ring[RIGHT])
    {
	ring_off();
	return cur_ring[RIGHT] == NULL;
    }

    return true;
}

/** new_thing:
 * Return a new thing */
THING *
new_thing(void)
{
    THING *cur;
    int r;

    cur = new_item();
    cur->o_hplus = 0;
    cur->o_dplus = 0;
    strncpy(cur->o_damage, "0x0", sizeof(cur->o_damage));
    strncpy(cur->o_hurldmg, "0x0", sizeof(cur->o_hurldmg));
    cur->o_arm = 11;
    cur->o_count = 1;
    cur->o_group = 0;
    cur->o_flags = 0;
    /*
     * Decide what kind of object it will be
     * If we haven't had food for a while, let it be food.
     */
    switch (no_food > 3 ? 2 : pick_one(things, NUMTHINGS))
    {
	case 0:
	    cur->o_type = POTION;
	    cur->o_which = pick_one(pot_info, NPOTIONS);
	when 1:
	    cur->o_type = SCROLL;
	    cur->o_which = pick_one(scr_info, MAXSCROLLS);
	when 2:
	    cur->o_type = FOOD;
	    no_food = 0;
	    if (rnd(10) != 0)
		cur->o_which = 0;
	    else
		cur->o_which = 1;
	when 3:
	    init_weapon(cur, pick_one(weap_info, MAXWEAPONS));
	    if ((r = rnd(100)) < 10)
	    {
		cur->o_flags |= ISCURSED;
		cur->o_hplus -= rnd(3) + 1;
	    }
	    else if (r < 15)
		cur->o_hplus += rnd(3) + 1;
	when 4:
	    cur->o_type = ARMOR;
	    cur->o_which = pick_one(arm_info, MAXARMORS);
	    cur->o_arm = a_class[cur->o_which];
	    if ((r = rnd(100)) < 20)
	    {
		cur->o_flags |= ISCURSED;
		cur->o_arm += rnd(3) + 1;
	    }
	    else if (r < 28)
		cur->o_arm -= rnd(3) + 1;
	when 5:
	    cur->o_type = RING;
	    cur->o_which = pick_one(ring_info, MAXRINGS);
	    switch (cur->o_which)
	    {
		case R_ADDSTR:
		case R_PROTECT:
		case R_ADDHIT:
		case R_ADDDAM:
		    if ((cur->o_arm = rnd(3)) == 0)
		    {
			cur->o_arm = -1;
			cur->o_flags |= ISCURSED;
		    }
		when R_AGGR:
		case R_TELEPORT:
		    cur->o_flags |= ISCURSED;
	    }
	when 6:
	    cur->o_type = STICK;
	    cur->o_which = pick_one(ws_info, MAXSTICKS);
	    fix_stick(cur);
	otherwise:
	    if (wizard)
		msg("Picked a bad kind of object");
	    wait_for(KEY_SPACE);
    }
    return cur;
}

/** pick_one:
 * Pick an item out of a list of nitems possible objects */
static int
pick_one(struct obj_info *info, int nitems)
{
    struct obj_info *end;
    struct obj_info *start;
    int i;

    start = info;
    for (end = &info[nitems], i = rnd(100); info < end; info++)
	if (i < info->oi_prob)
	    break;
    if (info == end)
    {
	if (wizard)
	{
	    msg("bad pick_one: %d from %d items", i, nitems);
	    for (info = start; info < end; info++)
		msg("%s: %d%%", info->oi_name, info->oi_prob);
	}
	info = start;
    }
    return (int)(info - start);
}

/** discovered_by_type
 * list what the player has discovered of this type */
static void
discovered_by_type(char type, struct obj_info *info, int max_items)
{
  int i;
  int items_found = 0;
  WINDOW *printscr = dupwin(stdscr);
  THING printable_object;
  coord orig_pos;

  getyx(stdscr, orig_pos.y, orig_pos.x);

  printable_object.o_type = type;
  printable_object.o_flags = 0;
  printable_object.o_count = 1;

  for (i = 0; i < max_items; ++i)
    if (info[i].oi_know || info[i].oi_guess)
    {
      printable_object.o_which = i;
      mvwprintw(printscr, ++items_found, 1,
                "%s", inv_name(&printable_object, false));
    }

  if (items_found == 0)
  {
    const char *type_as_str = NULL;
    switch (type)
    {
      case POTION: type_as_str = "potion";
      when SCROLL: type_as_str = "scroll";
      when RING:   type_as_str = "ring";
      when STICK:  type_as_str = "stick";
    }
    mvwprintw(printscr, 1, 1, (terse
          ? "No known %s"
          : "Haven't discovered anything about any %s"),
        type_as_str);
  }

  move(orig_pos.y, orig_pos.x);
  wrefresh(printscr);
  delwin(printscr);
}

/** discovered:
 * list what the player has discovered in this game of a certain type */
void
discovered(void)
{
  msg((terse
      ? "what type? (%c%c%c%c) "
      : "for what type of objects do you want a list? (%c%c%c%c) "),
      POTION, SCROLL, RING, STICK);
  while (true)
  {
    int ch = readchar();
    touchwin(stdscr);
    refresh();
    switch (ch)
    {
      case POTION: discovered_by_type(ch, pot_info, NPOTIONS);
      when SCROLL: discovered_by_type(ch, scr_info, MAXSCROLLS);
      when RING: discovered_by_type(ch, ring_info, MAXRINGS);
      when STICK: discovered_by_type(ch, ws_info, MAXSTICKS);
      otherwise: msg(""); return;
    }
  }

  touchwin(stdscr);
  msg("");
}

/** nameit:
 * Give the proper name to a potion, stick, or ring */
static void
nameit(THING *obj, const char *type, const char *which, struct obj_info *op,
    char *(*prfunc)(THING *))
{
    char *pb;

    if (op->oi_know || op->oi_guess)
    {
	if (obj->o_count == 1)
	    sprintf(prbuf, "A %s ", type);
	else
	    sprintf(prbuf, "%d %ss ", obj->o_count, type);
	pb = &prbuf[strlen(prbuf)];
	if (op->oi_know)
	    sprintf(pb, "of %s%s(%s)", op->oi_name, (*prfunc)(obj), which);
	else if (op->oi_guess)
	    sprintf(pb, "called %s%s(%s)", op->oi_guess, (*prfunc)(obj), which);
    }
    else if (obj->o_count == 1)
	sprintf(prbuf, "A%s %s %s", vowelstr(which), which, type);
    else
	sprintf(prbuf, "%d %s %ss", obj->o_count, which, type);
}

/** nullstr:
 * Return a pointer to a null-length string */
static char *
nullstr(THING *ignored)
{
    (void)ignored;
    return "";
}

