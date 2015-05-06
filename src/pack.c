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
#include <assert.h>

#include "scrolls.h"
#include "io.h"
#include "armor.h"
#include "potions.h"
#include "list.h"
#include "rings.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "weapons.h"
#include "wand.h"
#include "rogue.h"

#include "pack.h"

static THING *player_pack;

static struct equipment_t
{
  THING *ptr;
  const char *description;
} equipment[NEQUIPMENT] = {
  { NULL, "Body" },
  { NULL, "Right Hand" },
  { NULL, "Right Ring" },
  { NULL, "Left Ring" }
};

bool pack_used[26] = {			/* Is the character used in the pack? */
    false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false
};

enum equipment_pos ring_slots[RING_SLOTS_SIZE] = { 
  EQUIPMENT_RRING,
  EQUIPMENT_LRING
};

void *__pack_ptr(void) { return &player_pack; }

static size_t
pack_print_evaluate_item(THING *obj)
{
  int worth = 0;
  struct obj_info *op;
  if (obj == NULL)
    return 0;

  switch (obj->o_type)
  {
    case FOOD:
      worth = 2 * obj->o_count;
      break;
    case WEAPON:
      worth = weap_info[obj->o_which].oi_worth;
      worth *= 3 * (obj->o_hplus + obj->o_dplus) + obj->o_count;
      obj->o_flags |= ISKNOW;
      break;
    case ARMOR:
      worth = armor_value(obj->o_which);
      worth += (9 - obj->o_arm) * 100;
      worth += (10 * (armor_ac(obj->o_which) - obj->o_arm));
      obj->o_flags |= ISKNOW;
      break;
    case SCROLL:
      worth = scr_info[obj->o_which].oi_worth;
      worth *= obj->o_count;
      op = &scr_info[obj->o_which];
      if (!op->oi_know)
        worth /= 2;
      op->oi_know = true;
      break;
    case POTION:
      worth = pot_info[obj->o_which].oi_worth;
      worth *= obj->o_count;
      op = &pot_info[obj->o_which];
      if (!op->oi_know)
        worth /= 2;
      op->oi_know = true;
      break;
    case RING:
      op = &ring_info[obj->o_which];
      worth = op->oi_worth;
      if (obj->o_which == R_ADDSTR || obj->o_which == R_ADDDAM ||
          obj->o_which == R_PROTECT || obj->o_which == R_ADDHIT)
      {
        if (obj->o_arm > 0)
          worth += obj->o_arm * 100;
        else
          worth = 10;
      }
      if (!(obj->o_flags & ISKNOW))
        worth /= 2;
      obj->o_flags |= ISKNOW;
      op->oi_know = true;
      break;
    case STICK:
      wand_get_worth(obj->o_which);
      worth += 20 * obj->o_charges;
      if (!(obj->o_flags & ISKNOW))
        worth /= 2;
      obj->o_flags |= ISKNOW;
      wand_set_known(obj->o_which);
      break;
    case AMULET:
      worth = 1000;
      break;
  }

  if (worth < 0)
    worth = 0;

  printw("%5d  %s\n", worth,
      inv_name(obj, false));

  return (unsigned) worth;
}

static char
pack_char(void)
{
  bool *bp;
  for (bp = pack_used; *bp; bp++)
    ;
  *bp = true;
  return (char)((int)(bp - pack_used) + 'a');
}

static void
pack_move_msg(THING *obj)
{
  if (!terse)
    addmsg("you ");
  msg("moved onto %s", inv_name(obj, true));
}

static void
pack_add_money(int value)
{
  coord *player_pos = player_get_pos();
  purse += value;
  mvaddcch(player_pos->y, player_pos->x, floor_ch());
  chat(player_pos->y, player_pos->x) = (player_get_room()->r_flags & ISGONE)
    ? PASSAGE : FLOOR;
  if (value > 0)
  {
    if (!terse)
      addmsg("you found ");
    msg("%d gold pieces", value);
  }
}

static void
pack_remove_from_floor(THING *obj)
{
  coord *player_pos = player_get_pos();
  detach(lvl_obj, obj);
  mvaddcch(player_pos->y, player_pos->x, floor_ch());
  chat(player_pos->y, player_pos->x) = (player_get_room()->r_flags & ISGONE)
    ? PASSAGE : FLOOR;
}

bool
pack_add(THING *obj, bool silent)
{
  coord *player_pos = player_get_pos();
  THING *op;
  bool from_floor = false;

  /* Either obj in an item or we try to take something from the floor */
  if (obj == NULL)
  {
    if ((obj = find_obj(player_pos->y, player_pos->x)) == NULL)
      return false;
    from_floor = true;
  }

  /* Check for and deal with scare monster scrolls */
  if (obj->o_type == SCROLL && obj->o_which == S_SCARE &&
      obj->o_flags & ISFOUND)
  {
    detach(lvl_obj, obj);
    mvaddcch(player_pos->y, player_pos->x, floor_ch());
    chat(player_pos->y, player_pos->x) = (player_get_room()->r_flags & ISGONE)
      ? PASSAGE : FLOOR;
    discard(obj);
    msg("the scroll turns to dust as you pick it up");
    return false;
  }

  if (pack_count_items() == PACKSIZE)
  {
    msg(terse
        ? "no room"
        : "there's no room in your pack");
    if (from_floor)
      pack_move_msg(obj);
    return false;
  }

  if (player_pack == NULL)
  {
    if (from_floor)
      pack_remove_from_floor(obj);
    attach(player_pack, obj);
    obj->o_packch = pack_char();
  }
  else
  {
    THING *lp = NULL;
    for (op = player_pack; op != NULL; op = op->l_next)
    {
      if (op->o_type != obj->o_type)
        lp = op;
      else
      {
        while (op->o_type == obj->o_type && op->o_which != obj->o_which)
        {
          lp = op;
          if (op->l_next == NULL)
            break;
          else
            op = op->l_next;
        }
        if (op->o_type == obj->o_type && op->o_which == obj->o_which)
        {
          if (op->o_type == POTION || op->o_type == SCROLL ||
              obj->o_type == FOOD)
          {
            if (from_floor)
              pack_remove_from_floor(obj);
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
              if (op->l_next == NULL)
                break;
              else
                op = op->l_next;
            }
            if (op->o_type == obj->o_type
                && op->o_which == obj->o_which
                && op->o_group == obj->o_group)
            {
              op->o_count += obj->o_count;
              if (from_floor)
                pack_remove_from_floor(obj);
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
        pack_remove_from_floor(obj);
      obj->o_packch = pack_char();
      obj->l_next = lp->l_next;
      obj->l_prev = lp;
      if (lp->l_next != NULL)
        lp->l_next->l_prev = obj;
      lp->l_next = obj;
    }
  }

  obj->o_flags |= ISFOUND;

  /*
   * If this was the object of something's desire, that monster will
   * get mad and run at the hero.
   */
  for (op = mlist; op != NULL; op = op->l_next)
    if (op->t_dest == &obj->o_pos)
      op->t_dest = player_pos;

  /* Notify the user */
  if (!silent)
  {
    if (!terse)
      addmsg("you now have ");
    msg("%s (%c)", inv_name(obj, !terse), obj->o_packch, true);
  }
  return true;
}

THING *
pack_remove(THING *obj, bool newobj, bool all)
{
  THING *nobj = obj;
  if (obj->o_count > 1 && !all)
  {
    last_pick = obj;
    obj->o_count--;
    if (newobj)
    {
      nobj = new_item();
      *nobj = *obj;
      nobj->l_next = NULL;
      nobj->l_prev = NULL;
      nobj->o_count = 1;
    }
  }
  else
  {
    last_pick = NULL;
    pack_used[obj->o_packch - 'a'] = false;
    detach(player_pack, obj);
  }
  return nobj;
}


void
pack_pick_up(char ch)
{
  THING *obj;
  coord *player_pos = player_get_pos();

  if (player_is_levitating())
    return;

  obj = find_obj(player_pos->y, player_pos->x);
  if (move_on)
    pack_move_msg(obj);
  else
    switch (ch)
    {
      case GOLD:
        if (obj != NULL)
        {
          pack_add_money(obj->o_goldval);
          detach(lvl_obj, obj);
          discard(obj);
          player_get_room()->r_goldval = 0;
        }
        break;
      default:
        msg("DEBUG: You picked something you shouldn't have...");
        /* FALLTHROUGH */
      case ARMOR: case POTION: case FOOD: case WEAPON:
      case SCROLL: case AMULET: case RING: case STICK:
        pack_add((THING *) NULL, false);
        break;
    }
}


THING *
pack_find_magic_item(void)
{
  THING *obj = NULL;
  int nobj = 0;

  for (obj = player_pack; obj != NULL; obj = obj->l_next)
    if (is_magic(obj) && rnd(++nobj) == 0)
      return obj;
  return NULL;
}

THING *
pack_get_item(const char *purpose, int type)
{
  THING *obj;
  char ch;

  if (again)
    if (last_pick)
      return last_pick;
    else
    {
      msg("you ran out");
      return NULL;
    }

  /* Make sure theres an item of the type */
  else if (pack_count_items_of_type(type) == 0)
  {
    msg("You have no item to %s", purpose);
    return NULL;
  }

  msg(terse
      ? "%s what? "
      : "which object do you want to %s? ",
      purpose);
  pack_print_inventory(type);
  ch = readchar();
  mpos = 0;

  pack_clear_inventory();

  if (ch == KEY_ESCAPE)
  {
    reset_last();
    msg("");
    return NULL;
  }

  for (obj = player_pack; obj != NULL; obj = obj->l_next)
    if (obj->o_packch == ch)
      return obj;

  msg("'%s' is not a valid item",unctrl(ch));
  return NULL;
}

bool
pack_is_empty(void)
{
  return player_pack == NULL;
}

unsigned
pack_count_items(void)
{
  return pack_count_items_of_type(0);
}

unsigned
pack_count_items_of_type(int type)
{
  unsigned num = 0;
  THING *list;

  for (list = player_pack; list != NULL; list = list->l_next)
    if (!type || type == list->o_type ||
        (type == RENAMEABLE && (list->o_type != FOOD && list->o_type != AMULET))||
        (type == R_OR_S && (list->o_type == RING || list->o_type == STICK)))
      ++num;
  return num;
}

bool
pack_contains_amulet(void)
{
  THING *ptr;

  for (ptr = player_pack; ptr != NULL; ptr = ptr->l_next)
    if (ptr->o_type == AMULET)
      return true;
  return false;
}

bool
pack_contains(THING *item)
{
  THING *ptr;

  for (ptr = player_pack; ptr != NULL; ptr = ptr->l_next)
    if (ptr == item)
      return true;
  return false;
}

bool
pack_print_equipment(void)
{
  WINDOW *equipscr = dupwin(stdscr);
  char sym = 'a';
  size_t i;
  coord orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  for (i = 0; i < NEQUIPMENT; ++i)
  {
    if (equipment[i].ptr != NULL)
    {
      mvwprintw(equipscr, sym - 'a' + 1, 1, "%c) %s: %s",
                sym, equipment[i].description,
                inv_name(equipment[i].ptr, false));
      sym++;
    }
  }

  move(orig_pos.y, orig_pos.x);
  wrefresh(equipscr);
  delwin(equipscr);
  msg("--Press any key to continue--");
  readchar();
  touchwin(stdscr);
  msg("");
  return false;
}

bool
pack_print_inventory(int type)
{
  unsigned num_items = 0;
  THING *list;
  WINDOW *invscr = dupwin(stdscr);
  coord orig_pos;

  getyx(stdscr, orig_pos.y, orig_pos.x);

  /* Print out all items */
  for (list = player_pack; list != NULL; list = list->l_next)
  {
    if (!type || type == list->o_type ||
        (type == RENAMEABLE && (list->o_type != FOOD && list->o_type != AMULET))||
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
pack_clear_inventory(void)
{
  touchwin(stdscr);
}

size_t
pack_evaluate(void)
{
  size_t value = 0;
  int i;
  THING *obj = NULL;

  clear();
  mvaddstr(0, 0, "Worth  Item  [Equipment]\n");
  for (i = 0; i < NEQUIPMENT; ++i)
    value += pack_print_evaluate_item(pack_equipped_item(i));

  addstr("\nWorth  Item  [Inventory]\n");
  for (obj = player_pack; obj != NULL; obj = obj->l_next)
    value += pack_print_evaluate_item(obj);

  printw("\n%5d  Gold Pieces          ", purse);
  refresh();
  return value;
}

THING *
pack_equipped_item(enum equipment_pos pos)
{
  assert (pos >= 0 && pos < (sizeof equipment / sizeof (*equipment)));
  return equipment[pos].ptr;
}

bool
pack_equip_item(THING *item)
{
  enum equipment_pos pos;
  switch(item->o_type)
  {
    case ARMOR:
      pos = EQUIPMENT_ARMOR;
      break;
    case WEAPON:
      pos = EQUIPMENT_RHAND;
      break;
    case RING:
      pos = equipment[EQUIPMENT_RRING].ptr == NULL
        ? EQUIPMENT_RRING
        : EQUIPMENT_LRING;
      break;
    default:
      pos = EQUIPMENT_RHAND;
      break;
  }

  if (equipment[pos].ptr)
    return false;
  else
  {
    equipment[pos].ptr = item;
    return true;
  }
}

bool
pack_unequip(enum equipment_pos pos, bool quiet_on_success)
{
  THING *obj = pack_equipped_item(pos);
  const char *doing = pos == EQUIPMENT_RHAND ? "wielding" : "wearing";
  if (obj == NULL)
  {
    msg("not %s anything!", doing);
    return false;
  }

  if (obj->o_flags & ISCURSED)
  {
    msg("you can't. It appears to be cursed");
    return false;
  }

  equipment[pos].ptr = NULL;

  /* Waste time if armor - since they take a while */
  if (pos == EQUIPMENT_ARMOR)
    waste_time(1);

  if (!pack_add(obj, true))
  {
    coord *player_pos = player_get_pos();
    attach(lvl_obj, obj);
    chat(player_pos->y, player_pos->x) = (char) obj->o_type;
    flat(player_pos->y, player_pos->x) |= F_DROPPED;
    obj->o_pos = *player_pos;
    msg("dropped %s", inv_name(obj, true));
  }
  else if (!quiet_on_success)
    msg("no longer %s %s", doing, inv_name(obj, true));
  return true;
}

bool pack_item_is_cursed(THING *item)     { return item->t_flags & ISCURSED; }
void pack_curse_item(THING *item)         { item->t_flags |= ISCURSED; }
void pack_uncurse_item(THING *item)       { item->t_flags &= ~ISCURSED; }
