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
#include "armor.h"
#include "potions.h"

#include "pack.h"

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

enum equipment_pos ring_slots[RING_SLOTS_SIZE] = { 
  EQUIPMENT_RRING,
  EQUIPMENT_LRING
};

/* TODO: Maybe inline money()? */

static char pack_char(void);      /* Return the next unused pack character */
static void move_msg(THING *obj); /* Print out what you are moving onto */
static void money(int value);     /* Add or subtract gold from the pack */
static char floor_ch(void);       /* Return the appropriate floor character */
static void remove_from_floor(THING *obj); /* Removes one item from the floor */

bool
add_pack(THING *obj, bool silent)
{
  THING *op;
  bool from_floor = false;

  /* Either obj in an item or we try to take something from the floor */
  if (obj == NULL)
  {
    if ((obj = find_obj(hero.y, hero.x)) == NULL)
      return false;
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
    return false;
  }

  if (items_in_pack() == PACKSIZE)
  {
    msg(terse
        ? "no room"
        : "there's no room in your pack");
    if (from_floor)
      move_msg(obj);
    return false;
  }

  if (player.t_pack == NULL)
  {
    if (from_floor)
      remove_from_floor(obj);
    attach(player.t_pack, obj);
    obj->o_packch = pack_char();
  }
  else
  {
    THING *lp = NULL;
    for (op = player.t_pack; op != NULL; op = op->l_next)
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
      op->t_dest = &hero;

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
leave_pack(THING *obj, bool newobj, bool all)
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
    detach(player.t_pack, obj);
  }
  return nobj;
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
        if (obj != NULL)
        {
          money(obj->o_goldval);
          detach(lvl_obj, obj);
          discard(obj);
          proom->r_goldval = 0;
        }
      otherwise:
        msg("DEBUG: You picked something you shouldn't have...");
        /* FALLTHROUGH */
      case ARMOR: case POTION: case FOOD: case WEAPON:
      case SCROLL: case AMULET: case RING: case STICK:
        add_pack((THING *) NULL, false);
        break;
    }
}

static void
move_msg(THING *obj)
{
  if (!terse)
    addmsg("you ");
  msg("moved onto %s", inv_name(obj, true));
}

THING *
find_magic_item_in_players_pack(void)
{
  THING *obj = NULL;
  int nobj = 0;

  for (obj = player.t_pack; obj != NULL; obj = obj->l_next)
    if (is_magic(obj) && rnd(++nobj) == 0)
      return obj;
  return NULL;
}

THING *
get_item(const char *purpose, int type)
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
  else if (items_in_pack_of_type(type) == 0)
  {
    msg("You have no item to %s", purpose);
    return NULL;
  }

  msg(terse
      ? "%s what? "
      : "which object do you want to %s? ",
      purpose);
  print_inventory(type);
  ch = readchar();
  mpos = 0;

  clear_inventory();

  if (ch == KEY_ESCAPE)
  {
    reset_last();
    msg("");
    return NULL;
  }

  for (obj = player.t_pack; obj != NULL; obj = obj->l_next)
    if (obj->o_packch == ch)
      return obj;

  msg("'%s' is not a valid item",unctrl(ch));
  return NULL;
}

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

static char
floor_ch(void)
{
  if (proom->r_flags & ISGONE)
    return PASSAGE;
  return show_floor() ? FLOOR : SHADOW;
}

char
floor_at(void)
{
  char ch = chat(hero.y, hero.x);
  return ch == FLOOR ? floor_ch() : ch;
}

void
reset_last(void)
{
  last_comm = l_last_comm;
  last_dir = l_last_dir;
  last_pick = l_last_pick;
}

static void
remove_from_floor(THING *obj)
{
  detach(lvl_obj, obj);
  mvaddcch(hero.y, hero.x, floor_ch());
  chat(hero.y, hero.x) = (proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
}

unsigned
items_in_pack_of_type(int type)
{
  unsigned num = 0;
  THING *list;

  for (list = player.t_pack; list != NULL; list = list->l_next)
    if (!type || type == list->o_type ||
        (type == RENAMEABLE && (list->o_type != FOOD && list->o_type != AMULET))||
        (type == R_OR_S && (list->o_type == RING || list->o_type == STICK)))
      ++num;
  return num;
}

bool
player_has_amulet(void)
{
  THING *ptr;

  for (ptr = player.t_pack; ptr != NULL; ptr = ptr->l_next)
    if (ptr->o_type == AMULET)
      return true;
  return false;
}

bool
print_equipment(void)
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
print_inventory(int type)
{
  unsigned num_items = 0;
  THING *list;
  WINDOW *invscr = dupwin(stdscr);
  coord orig_pos;

  getyx(stdscr, orig_pos.y, orig_pos.x);

  /* Print out all items */
  for (list = player.t_pack; list != NULL; list = list->l_next)
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

static size_t
print_evaluate_item(THING *obj)
{
  int worth = 0;
  struct obj_info *op;
  if (obj == NULL)
    return 0;

  switch (obj->o_type)
  {
    case FOOD:
      worth = 2 * obj->o_count;
    when WEAPON:
      worth = weap_info[obj->o_which].oi_worth;
      worth *= 3 * (obj->o_hplus + obj->o_dplus) + obj->o_count;
      obj->o_flags |= ISKNOW;
    when ARMOR:
      worth = armor_value(obj->o_which);
      worth += (9 - obj->o_arm) * 100;
      worth += (10 * (armor_ac(obj->o_which) - obj->o_arm));
      obj->o_flags |= ISKNOW;
    when SCROLL:
      worth = scr_info[obj->o_which].oi_worth;
      worth *= obj->o_count;
      op = &scr_info[obj->o_which];
      if (!op->oi_know)
        worth /= 2;
      op->oi_know = true;
    when POTION:
      worth = pot_info[obj->o_which].oi_worth;
      worth *= obj->o_count;
      op = &pot_info[obj->o_which];
      if (!op->oi_know)
        worth /= 2;
      op->oi_know = true;
    when RING:
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
    when STICK:
      op = &ws_info[obj->o_which];
      worth = op->oi_worth;
      worth += 20 * obj->o_charges;
      if (!(obj->o_flags & ISKNOW))
        worth /= 2;
      obj->o_flags |= ISKNOW;
      op->oi_know = true;
    when AMULET:
      worth = 1000;
  }

  if (worth < 0)
    worth = 0;

  printw("%5d  %s\n", worth,
      inv_name(obj, false));

  return (unsigned) worth;
}

size_t
evaluate_players_inventory(void)
{
  size_t value = 0;
  int i;
  THING *obj = NULL;

  clear();
  mvaddstr(0, 0, "Worth  Item  [Equipment]\n");
  for (i = 0; i < NEQUIPMENT; ++i)
    value += print_evaluate_item(equipped_item(i));

  addstr("\nWorth  Item  [Inventory]\n");
  for (obj = player.t_pack; obj != NULL; obj = obj->l_next)
    value += print_evaluate_item(obj);

  printw("\n%5d  Gold Pieces          ", purse);
  refresh();
  return value;
}

void
clear_inventory(void)
{
  touchwin(stdscr);
}

THING *
equipped_item(enum equipment_pos pos)
{
  return equipment[pos].ptr;
}

bool
equip_item(THING *item)
{
  enum equipment_pos pos;
  switch(item->o_type)
  {
    case ARMOR:  pos = EQUIPMENT_ARMOR;
    when WEAPON: pos = EQUIPMENT_RHAND;
    when RING: pos = equipment[EQUIPMENT_RRING].ptr == NULL
                     ? EQUIPMENT_RRING
                     : EQUIPMENT_LRING;
    otherwise:   pos = EQUIPMENT_RHAND;
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
unequip_item(enum equipment_pos pos)
{
  THING *obj = equipped_item(pos);
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

  if (!add_pack(obj, true))
  {
    attach(lvl_obj, obj);
    chat(hero.y, hero.x) = (char) obj->o_type;
    flat(hero.y, hero.x) |= F_DROPPED;
    obj->o_pos = hero;
    msg("dropped %s", inv_name(obj, true));
  }
  else
    msg("no longer %s %s", doing, inv_name(obj, true));
  return true;
}
