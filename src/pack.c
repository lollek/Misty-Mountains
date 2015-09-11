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
#include "things.h"
#include "monster.h"
#include "os.h"
#include "rooms.h"
#include "state.h"
#include "options.h"
#include "rogue.h"
#include "item.h"

#include "pack.h"

int const PACK_RENAMEABLE = -1;

int           pack_gold = 0;
static THING* player_pack;

static struct equipment_t
{
  THING* ptr;
  char const* description;
} equipment[NEQUIPMENT] = {
  { NULL, "Body" },
  { NULL, "Right Hand" },
  { NULL, "Right Ring" },
  { NULL, "Left Ring" }
};

/* Is the character used in the pack? */
static bool pack_used[26];

enum equipment_pos pack_ring_slots[PACK_RING_SLOTS] = {
  EQUIPMENT_RRING,
  EQUIPMENT_LRING
};

bool
pack_save_state(void)
{
  return state_save_list(player_pack)
    || state_save_bools(pack_used, sizeof(pack_used)/sizeof(*pack_used));
}

bool
pack_load_state(void)
{
  return state_load_list(&player_pack)
    || state_load_bools(pack_used, sizeof(pack_used)/sizeof(*pack_used));
}

int8_t
pack_list_index(item const* thing)
{
  if (thing == NULL)
    return -1;

  item const* ptr;
  int8_t i;
  for (ptr = &player_pack->o, i = 0; ptr != NULL; ptr = &ptr->l_next->o, ++i)
    if (ptr == thing)
      return i;

  return -1;
}

THING *
pack_list_element(int8_t i)
{
  if (i < 0)
    return NULL;

  for (THING* ptr = player_pack; ptr != NULL; ptr = ptr->o.l_next)
    if (i-- == 0)
      return ptr;

  return NULL;
}

int
pack_size(void)
{
  return 22;
}

static size_t
pack_print_evaluate_item(THING* obj)
{
  int worth = 0;
  struct obj_info *op;
  if (obj == NULL)
    return 0;

  switch (obj->o.o_type)
  {
    case FOOD:
      worth = 2 * obj->o.o_count;
      break;

    case WEAPON: case AMMO:
      worth = weapon_info[obj->o.o_which].oi_worth;
      worth *= 3 * (obj->o.o_hplus + obj->o.o_dplus) + obj->o.o_count;
      obj->o.o_flags |= ISKNOW;
      break;

    case ARMOR:
      worth = armor_value((enum armor_t)obj->o.o_which);
      worth += (9 - obj->o.o_arm) * 100;
      worth += (10 * (armor_ac((enum armor_t)obj->o.o_which) - obj->o.o_arm));
      obj->o.o_flags |= ISKNOW;
      break;

    case SCROLL:
      {
        enum scroll_t scroll = (enum scroll_t)obj->o.o_which;
        worth = scroll_value(scroll);
        worth *= obj->o.o_count;
        if (!scroll_is_known(scroll))
          worth /= 2;
        scroll_learn(scroll);
      }
      break;

    case POTION:
      worth = potion_info[obj->o.o_which].oi_worth;
      worth *= obj->o.o_count;
      op = &potion_info[obj->o.o_which];
      if (!op->oi_know)
        worth /= 2;
      op->oi_know = true;
      break;

    case RING:
      op = &ring_info[obj->o.o_which];
      worth = op->oi_worth;
      if (obj->o.o_which == R_ADDSTR || obj->o.o_which == R_ADDDAM ||
          obj->o.o_which == R_PROTECT || obj->o.o_which == R_ADDHIT)
      {
        if (obj->o.o_arm > 0)
          worth += obj->o.o_arm * 100;
        else
          worth = 10;
      }
      if (!(obj->o.o_flags & ISKNOW))
        worth /= 2;
      obj->o.o_flags |= ISKNOW;
      op->oi_know = true;
      break;

    case STICK:
      wand_get_worth((enum wand_t)obj->o.o_which);
      worth += 20 * obj->o.o_charges;
      if (!(obj->o.o_flags & ISKNOW))
        worth /= 2;
      obj->o.o_flags |= ISKNOW;
      wand_set_known((enum wand_t)obj->o.o_which);
      break;

    case AMULET:
      worth = 1000;
      break;

    default:
      io_debug_fatal("Unknown type: %c(%d)", obj->o.o_type, obj->o.o_type);
      break;
  }

  if (worth < 0)
    worth = 0;

  char buf[MAXSTR];
  printw("%5d  %s\n", worth,
      inv_name(buf, obj, false));

  return (unsigned) worth;
}

static char
pack_char(void)
{
  bool* bp;
  for (bp = pack_used; *bp; bp++)
    ;
  *bp = true;
  return (char)((int)(bp - pack_used) + 'a');
}

void
pack_move_msg(THING* obj)
{
  char buf[MAXSTR];
  io_msg("moved onto %s", inv_name(buf, obj, true));
}

static void
pack_add_money(int value)
{
  coord const* player_pos = player_get_pos();
  pack_gold += value;

  mvaddcch(player_pos->y, player_pos->x, (chtype) floor_ch());
  level_set_ch(player_pos->y, player_pos->x,
      (player_get_room()->r_flags & ISGONE)
        ? PASSAGE
        : FLOOR);

  if (value > 0)
    io_msg("you found %d gold pieces", value);
}

static void
pack_remove_from_floor(THING* obj)
{
  coord const* player_pos = player_get_pos();

  list_detach(&level_items, obj);
  mvaddcch(player_pos->y, player_pos->x, (chtype) floor_ch());
  level_set_ch(player_pos->y, player_pos->x,
      (player_get_room()->r_flags & ISGONE)
        ? PASSAGE
        : FLOOR);
}

bool
pack_add(THING* obj, bool silent)
{
  coord* player_pos = player_get_pos();
  bool from_floor = false;
  bool is_picked_up = false;

  /* Either obj is an item or we try to take something from the floor */
  if (obj == NULL)
  {
    obj = find_obj(player_pos->y, player_pos->x);
    if (obj == NULL)
      return false;
    from_floor = true;
  }

  /* Check for and deal with scare monster scrolls */
  if (obj->o.o_type == SCROLL && obj->o.o_which == S_SCARE && obj->o.o_flags & ISFOUND)
  {
    list_detach(&level_items, obj);
    mvaddcch(player_pos->y, player_pos->x, (chtype) floor_ch());
    level_set_ch(player_pos->y, player_pos->x, floor_ch());
    os_remove_thing(&obj);
    io_msg("the scroll turns to dust as you pick it up");
    return false;
  }

  /* See if we can stack it with something else in the pack */
  if (obj->o.o_type == POTION || obj->o.o_type == SCROLL || obj->o.o_type == FOOD
      || obj->o.o_type == AMMO)
    for (THING* ptr = player_pack; ptr != NULL; ptr = ptr->o.l_next)
      if (ptr->o.o_type == obj->o.o_type && ptr->o.o_which == obj->o.o_which
          && ptr->o.o_hplus == obj->o.o_hplus && ptr->o.o_dplus == obj->o.o_dplus)
      {
        if (from_floor)
          pack_remove_from_floor(obj);
        ptr->o.o_count += obj->o.o_count;
        ptr->o.o_pos = obj->o.o_pos;
        os_remove_thing(&obj);
        obj = ptr;
        is_picked_up = true;
        break;
      }

  /* If we cannot stack it, we need to have available space in the pack */
  if (!is_picked_up && pack_count_items() == pack_size())
  {
    io_msg("there's no room in your pack");
    if (from_floor)
      pack_move_msg(obj);
    return false;
  }

  /* Empty pack? Just insert it */
  if (!is_picked_up && player_pack == NULL)
  {
    if (from_floor)
      pack_remove_from_floor(obj);
    list_attach(&player_pack, obj);
    obj->o.o_packch = pack_char();
    is_picked_up = true;
  }

  /* Add thing to inventory in a sorted way */
  else if (!is_picked_up)
  {
    THING* prev_ptr = NULL;
    THING* ptr = player_pack;

    /* Try to find an object of the same type */
    while (ptr != NULL && ptr->o.o_type != obj->o.o_type)
    {
      prev_ptr = ptr;
      ptr = ptr->o.l_next;
    }

    /* Move to the end of those objects, or stop if found similar item */
    while (ptr != NULL && ptr->o.o_type == obj->o.o_type
           && ptr->o.o_which != obj->o.o_which)
    {
      prev_ptr = ptr;
      ptr = ptr->o.l_next;
    }

    /* Make object ready for insertion */
    if (from_floor)
      pack_remove_from_floor(obj);
    obj->o.o_packch = pack_char();

    /* Add to list */
    if (ptr == NULL && prev_ptr == NULL)
      list_attach(&player_pack, obj);
    else
    {
      obj->o.l_next = ptr;
      obj->o.l_prev = prev_ptr;
      if (ptr != NULL)
        ptr->o.l_prev = obj;
      if (prev_ptr == NULL)
        player_pack = obj;
      else
        prev_ptr->o.l_next = obj;
    }
  }

  obj->o.o_flags |= ISFOUND;

  /* If this was the object of something's desire, that monster will
   * get mad and run at the hero.  */
  for (THING* op = monster_list; op != NULL; op = op->t.l_next)
    if (op->t.t_dest == &obj->o.o_pos)
      op->t.t_dest = player_pos;

  /* Notify the user */
  if (!silent)
  {
    char buf[MAXSTR];
    io_msg("you now have %s (%c)", inv_name(buf, obj, true), obj->o.o_packch, true);
  }
  return true;
}

THING*
pack_remove(THING* obj, bool newobj, bool all)
{
  THING* nobj = obj;

  if (obj->o.o_count > 1 && !all)
  {
    obj->o.o_count--;
    if (newobj)
    {
      nobj = os_calloc_thing();
      *nobj = *obj;
      nobj->o.l_next = NULL;
      nobj->o.l_prev = NULL;
      nobj->o.o_count = 1;
    }
  }
  else
  {
    pack_used[obj->o.o_packch - 'a'] = false;
    list_detach(&player_pack, obj);
  }
  return nobj;
}


void
pack_pick_up(THING* obj, bool force)
{
  if (player_is_levitating())
    return;

  switch (obj->o.o_type)
  {
    case GOLD:
      if (obj != NULL)
      {
        pack_add_money(obj->o.o_goldval);
        list_detach(&level_items, obj);
        os_remove_thing(&obj);
        player_get_room()->r_goldval = 0;
      }
      return;

    case POTION: case WEAPON: case AMMO: case FOOD: case ARMOR:
    case SCROLL: case AMULET: case RING: case STICK:
      if (force || option_autopickup(obj->o.o_type))
        pack_add((THING *) NULL, false);
      else
        pack_move_msg(obj);
      return;
  }

  io_debug("Unknown type: %c(%d)", obj->o.o_type, obj->o.o_type);
  assert(0);
}


THING*
pack_find_magic_item(void)
{
  int nobj = 0;

  for (THING* obj = player_pack; obj != NULL; obj = obj->o.l_next)
    if (is_magic(obj) && os_rand_range(++nobj) == 0)
      return obj;
  return NULL;
}

THING*
pack_get_item(char const* purpose, int type)
{
  if (pack_count_items_of_type(type) < 1)
  {
    io_msg("You have no item to %s", purpose);
    return NULL;
  }

  pack_print_inventory(type);
  io_msg("which object do you want to %s? ", purpose);
  char ch = io_readchar(true);
  io_msg_clear();

  pack_clear_inventory();

  if (ch == KEY_ESCAPE || ch == KEY_SPACE)
  {
    io_msg_clear();
    return NULL;
  }

  for (THING* obj = player_pack; obj != NULL; obj = obj->o.l_next)
    if (obj->o.o_packch == ch)
      return obj;

  io_msg("'%s' is not a valid item",unctrl((chtype) ch));
  return NULL;
}

bool
pack_is_empty(void)
{
  return player_pack == NULL;
}

int
pack_count_items(void)
{
  return pack_count_items_of_type(0);
}

int
pack_count_items_of_type(int type)
{
  int num = 0;

  for (THING const* list = player_pack; list != NULL; list = list->o.l_next)
    if (!type || type == list->o.o_type ||
        (type == PACK_RENAMEABLE && (list->o.o_type != FOOD && list->o.o_type != AMULET)))
      ++num;
  return num;
}

bool
pack_contains_amulet(void)
{
  for (THING const* ptr = player_pack; ptr != NULL; ptr = ptr->o.l_next)
    if (ptr->o.o_type == AMULET)
      return true;
  return false;
}

bool
pack_contains(THING *item)
{
  for (THING const* ptr = player_pack; ptr != NULL; ptr = ptr->o.l_next)
    if (ptr == item)
      return true;
  return false;
}

bool
pack_print_equipment(void)
{
  char buf[MAXSTR];
  WINDOW* equipscr = dupwin(stdscr);

  coord orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  char sym = 'a';
  for (unsigned i = 0; i < NEQUIPMENT; ++i)
  {
    if (equipment[i].ptr != NULL)
    {
      mvwprintw(equipscr, sym - 'a' + 1, 1, "%c) %s: %s",
                sym, equipment[i].description,
                inv_name(buf, equipment[i].ptr, false));
      sym++;
    }
  }

  move(orig_pos.y, orig_pos.x);
  wrefresh(equipscr);
  delwin(equipscr);
  io_msg("--Press any key to continue--");
  io_readchar(false);
  touchwin(stdscr);
  io_msg_clear();
  return false;
}

bool
pack_print_inventory(int type)
{
  char buf[MAXSTR];
  WINDOW* invscr = dupwin(stdscr);

  coord orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  int num_items = 0;
  /* Print out all items */
  for (THING* list = player_pack; list != NULL; list = list->o.l_next)
  {
    if (!type || type == list->o.o_type ||
        (type == PACK_RENAMEABLE && (list->o.o_type != FOOD && list->o.o_type != AMULET)))
    {
      /* Print out the item and move to next row */
      wmove(invscr, ++num_items, 1);
      wprintw(invscr, "%c) %s", list->o.o_packch, inv_name(buf, list, false));
    }
  }

  wmove(stdscr, orig_pos.y, orig_pos.x);
  wrefresh(invscr);
  delwin(invscr);
  untouchwin(stdscr);
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

  clear();
  mvaddstr(0, 0, "Worth  Item  [Equipment]\n");
  for (int i = 0; i < NEQUIPMENT; ++i)
    value += pack_print_evaluate_item(pack_equipped_item((enum equipment_pos)i));

  addstr("\nWorth  Item  [Inventory]\n");
  for (THING* obj = player_pack; obj != NULL; obj = obj->o.l_next)
    value += pack_print_evaluate_item(obj);

  printw("\n%5d  Gold Pieces          ", pack_gold);
  refresh();
  return value;
}

THING*
pack_equipped_item(enum equipment_pos pos)
{
  assert (pos >= 0 && pos < (sizeof equipment / sizeof (*equipment)));
  return equipment[pos].ptr;
}

bool
pack_equip_item(THING* item)
{
  enum equipment_pos pos;
  switch(item->o.o_type)
  {
    case ARMOR:
      pos = EQUIPMENT_ARMOR;
      break;

    case WEAPON: case AMMO:
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
  char const* doing = pos == EQUIPMENT_RHAND
    ? "wielding"
    : "wearing";

  THING* obj = pack_equipped_item(pos);
  if (obj == NULL)
  {
    io_msg("not %s anything!", doing);
    return false;
  }

  if (obj->o.o_flags & ISCURSED)
  {
    io_msg("you can't. It appears to be cursed");
    return false;
  }

  equipment[pos].ptr = NULL;

  /* Waste time if armor - since they take a while */
  if (pos == EQUIPMENT_ARMOR)
    waste_time(1);

  char buf[MAXSTR];
  if (!pack_add(obj, true))
  {
    coord const* player_pos = player_get_pos();
    list_attach(&level_items, obj);
    level_set_ch(player_pos->y, player_pos->x, (char)obj->o.o_type);
    int flags = level_get_flags(player_pos->y, player_pos->x);
    flags |= F_DROPPED;
    level_set_flags(player_pos->y, player_pos->x, (char)flags);
    obj->o.o_pos = *player_pos;
    io_msg("dropped %s", inv_name(buf, obj, true));
  }
  else if (!quiet_on_success)
    io_msg("no longer %s %s", doing, inv_name(buf, obj, true));
  return true;
}


THING*
pack_find_arrow(void)
{
  for (THING* ptr = player_pack; ptr != NULL; ptr = ptr->o.l_next)
    if (ptr->o.o_which == ARROW)
      return ptr;
  return NULL;
}

static void
pack_identify_item_set_know(item* item, struct obj_info* info)
{
  int const subtype = item_subtype(item);
  info[subtype].oi_know = true;
  item->o_flags |= ISKNOW;

  char** guess = &info[subtype].oi_guess;
  if (*guess)
  {
    free(*guess);
    *guess = NULL;
  }
}

void
pack_identify_item(void)
{
  if (pack_is_empty())
  {
    io_msg("you don't have anything in your pack to identify");
    return;
  }

  THING* obj = pack_get_item("identify", 0);
  if (obj == NULL)
    return;

  switch (obj->o.o_type)
  {
    case SCROLL: pack_identify_item_set_know(&obj->o, scroll_info);  break;
    case POTION: pack_identify_item_set_know(&obj->o, potion_info);  break;
    case STICK:  pack_identify_item_set_know(&obj->o, __wands_ptr());   break;
    case RING:   pack_identify_item_set_know(&obj->o, ring_info); break;
    case WEAPON: case ARMOR: obj->o.o_flags |= ISKNOW; break;
    default: break;
  }

  char buf[MAXSTR];
  io_msg(inv_name(buf, obj, false));
}


