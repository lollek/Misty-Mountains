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

#include "potions.h"
#include "scrolls.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "list.h"
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

/* Only oi_prob is used
 *    oi_name  oi_prob oi_worth oi_guess oi_know */
struct obj_info things[] = {
    { "potion",	26,	0,	NULL,	false },	/* potion */
    { "scroll",	36,	0,	NULL,	false },	/* scroll */
    { "food",	16,	0,	NULL,	false },	/* food */
    { "weapon",	 7,	0,	NULL,	false },	/* weapon */
    { "armor",	 7,	0,	NULL,	false },	/* armor */
    { "ring",	 4,	0,	NULL,	false },	/* ring */
    { "stick",	 4,	0,	NULL,	false },	/* stick */
};

static const char *
type_to_string(int type, int which)
{
  switch (type)
  {
    case POTION: return "potion";
    case RING: return "ring";
    case STICK: return "stick";
    case SCROLL: return "scroll";
    case FOOD: return which == 1 ? "fruit" : "food ration";
    case WEAPON: return weap_info[which].oi_name;
    case ARMOR: return armor_name((enum armor_t)which);
    default: return "something";
  }
}

static int
add_num_type_to_string(char *ptr, int type, int which, int num)
{
  const char *obj_type = type_to_string(type, which);

  if (num == 1)
    return sprintf(ptr, "A%s %s", vowelstr(obj_type), obj_type);
  else
    return sprintf(ptr, "%d %ss", num, obj_type);
}

char *
inv_name(THING *obj, bool drop)
{
  static char buf[2*MAXSTR];
  char *pb = buf;
  int which = obj->o_which;

  switch (obj->o_type)
  {
    case POTION: potion_description(obj, buf); break;
    case RING: ring_description(obj, buf); break;
    case STICK: wand_description(obj, buf); break;
    case SCROLL:
      {
        struct obj_info *op = &scr_info[which];

        pb += add_num_type_to_string(pb, obj->o_type, obj->o_which, obj->o_count);

        if (op->oi_know)
          pb += sprintf(pb, " of %s", op->oi_name);
        else if (op->oi_guess)
          pb += sprintf(pb, " called %s", op->oi_guess);
        else
          pb += sprintf(pb, " titled '%s'", s_names[which]);
      }
      break;
    case FOOD:
      pb += add_num_type_to_string(pb, obj->o_type, obj->o_which, obj->o_count);
      break;
    case WEAPON:
      {
        pb += add_num_type_to_string(pb, obj->o_type, obj->o_which, obj->o_count);

        if (which != ARROW)
        {
          /* TODO: Maybe we can rename NumxNum -> NumdNum everywhere? */
          char damage[4] = { '\0' };
          sprintf(damage, "%s", which == BOW ? obj->o_hurldmg : obj->o_damage);
          damage[1] = 'd';
          pb += sprintf(pb, " (%s)", damage);
        }

        if (obj->o_flags & ISKNOW)
        {
          pb += sprintf(pb, " (");
          pb += sprintf(pb, obj->o_hplus < 0 ? "%d," : "+%d,", obj->o_hplus);
          pb += sprintf(pb, obj->o_dplus < 0 ? "%d)" : "+%d)", obj->o_dplus);
        }

        if (obj->o_arm != 0)
          pb += sprintf(pb, obj->o_arm < 0 ? " [%d]" : " [+%d]", obj->o_arm);

        if (obj->o_label != NULL)
          pb += sprintf(pb, " called %s", obj->o_label);
      }
      break;
    case ARMOR:
      {
        int bonus_ac = armor_ac((enum armor_t)which) - obj->o_arm;
        int base_ac = 10 - obj->o_arm - bonus_ac;

        pb += add_num_type_to_string(pb, obj->o_type, obj->o_which, obj->o_count);
        pb += sprintf(pb, " [%d]", base_ac);

        if (obj->o_flags & ISKNOW)
        {
          pb -= 1;
          pb += sprintf(pb, bonus_ac < 0 ? ",%d]" : ",+%d]", bonus_ac);
        }

        if (obj->o_label != NULL)
          pb += sprintf(pb, " called %s", obj->o_label);
      }
      break;
    case AMULET:
      pb += strlen(strcpy(pb, "The Amulet of Yendor"));
      break;
    case GOLD:
      pb += sprintf(pb, "%d Gold pieces", obj->o_goldval);
      break;
    default:
      msg("You feel a disturbance in the force");
      pb += sprintf(pb, "Something bizarre %s", unctrl((chtype)obj->o_type));
      break;
  }

  buf[0] = drop ? (char)tolower(buf[0]) : (char)toupper(buf[0]);
  buf[MAXSTR-1] = '\0';
  return buf;
}

bool
drop(void)
{
  coord *player_pos = player_get_pos();
  char ch = level_get_ch(player_pos->y, player_pos->x);
  THING *obj;

  if (ch != FLOOR && ch != PASSAGE)
  {
    msg("there is something there already");
    return false;
  }

  if ((obj = pack_get_item("drop", 0)) == NULL)
    return false;

  obj = pack_remove(obj, true, !(obj->o_type == POTION ||
        obj->o_type == SCROLL || obj->o_type == FOOD));

  /* Link it into the level object list */
  list_attach(&lvl_obj, obj);
  level_set_ch(player_pos->y, player_pos->x, (char)obj->o_type);
  int flags = level_get_flags(player_pos->y, player_pos->x);
  flags |= F_DROPPED;
  level_set_flags(player_pos->y, player_pos->x, (char)flags);
  obj->o_pos = *player_pos;
  msg("dropped %s", inv_name(obj, true));
  return true;
}

static THING *
new_generic_thing(void)
{
  THING *cur = allocate_new_item();

  assert (sizeof(cur->o_damage) >= sizeof("0x0"));
  assert (sizeof(cur->o_hurldmg) >= sizeof("0x0"));

  cur->o_hplus = 0;
  cur->o_dplus = 0;
  strcpy(cur->o_damage, "0x0");
  strcpy(cur->o_hurldmg, "0x0");
  cur->o_arm = 11;
  cur->o_count = 1;
  cur->o_group = 0;
  cur->o_flags = 0;

  return cur;
}

THING *
new_thing(void)
{
  THING *cur = NULL;
  int r;

  if (no_food > 3)
    r = 2;
  else
    r = (int)pick_one(things, NUMTHINGS);

  /* Decide what kind of object it will be
   * If we haven't had food for a while, let it be food. */
  switch (r)
  {
    case 0:
      cur = new_generic_thing();
      cur->o_type = POTION;
      cur->o_which = (int)pick_one(pot_info, NPOTIONS);
      break;
    case 1:
      cur = new_generic_thing();
      cur->o_type = SCROLL;
      cur->o_which = (int)pick_one(scr_info, NSCROLLS);
      break;
    case 2:
      cur = new_generic_thing();
      cur->o_type = FOOD;
      no_food = 0;
      cur->o_which = rnd(10) ? 0 : 1;
      break;
    case 3:
      cur = new_generic_thing();

      init_weapon(cur, (int)pick_one(weap_info, MAXWEAPONS));
      r = rnd(100);
      if (r < 10)
      {
        cur->o_flags |= ISCURSED;
        cur->o_hplus -= rnd(3) + 1;
      }
      else if (r < 15)
        cur->o_hplus += rnd(3) + 1;
      break;
    case 4:
      cur = new_generic_thing();

      cur->o_type = ARMOR;
      cur->o_which = armor_type_random();
      cur->o_arm = armor_ac((enum armor_t)cur->o_which);
      r = rnd(100);
      if (r < 20)
      {
        cur->o_flags |= ISCURSED;
        cur->o_arm += rnd(3) + 1;
      }
      else if (r < 28)
        cur->o_arm -= rnd(3) + 1;
      break;
    case 5:
      cur = new_generic_thing();
      cur->o_type = RING;
      cur->o_which = (int)pick_one(ring_info, NRINGS);
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
          break;
        case R_AGGR:
        case R_TELEPORT:
          cur->o_flags |= ISCURSED;
      }
      break;
    case 6: return wand_create(-1);
    default:
      msg("Picked a bad kind of object (this should not happen)");
      wait_for(KEY_SPACE);
      break;
  }
  assert(cur != NULL);
  return cur;
}

unsigned
pick_one(struct obj_info *start, int nitems)
{
  struct obj_info *ptr;
  struct obj_info *end = &start[nitems];
  int i = rnd(100);

  for (ptr = start ; ptr != end; ++ptr)
    if (i < ptr->oi_prob)
      return (unsigned)(ptr - start);
    else
      i -= ptr->oi_prob;

  /* The functions should have returned by now */
  {
    coord orig;
    int curr_y = 0;
    getyx(stdscr, orig.y, orig.x);
    mvprintw(0, 0, "DEBUG: bad pick_one: %d from %d items", i, nitems);
    for (ptr = start; ptr != end; ++ptr)
      mvprintw(++curr_y, 1, "%s: %d%%", ptr->oi_name, ptr->oi_prob);
    move(orig.y, orig.x);
    assert(0);
    return 0;
  }
}

/* list what the player has discovered of this type */
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
      case POTION: type_as_str = "potion"; break;
      case SCROLL: type_as_str = "scroll"; break;
      case RING:   type_as_str = "ring"; break;
      case STICK:  type_as_str = "stick"; break;
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

void
discovered(void)
{
  msg((terse
      ? "what type? (%c%c%c%c) "
      : "for what type of objects do you want a list? (%c%c%c%c) "),
      POTION, SCROLL, RING, STICK);
  while (true)
  {
    char ch = readchar(true);
    touchwin(stdscr);
    refresh();
    switch (ch)
    {
      case POTION: discovered_by_type(ch, pot_info, NPOTIONS); break;
      case SCROLL: discovered_by_type(ch, scr_info, NSCROLLS); break;
      case RING: discovered_by_type(ch, ring_info, NRINGS); break;
      case STICK: discovered_by_type(ch, __wands_ptr(), MAXSTICKS); break;
      default: clearmsg(); return;
    }
  }

  touchwin(stdscr);
  clearmsg();
}

