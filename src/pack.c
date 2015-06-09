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

#include "pack.h"

int const PACK_RENAMEABLE = -1;

int           pack_gold = 0;
static THING* player_pack;
static THING* last_picked_item;
static THING* last_last_picked_item;

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
pack_list_index(THING const* thing)
{
  if (thing == NULL)
    return -1;

  THING const* ptr;
  int8_t i;
  for (ptr = player_pack, i = 0; ptr != NULL; ptr = ptr->l_next, ++i)
    if (ptr == thing)
      return i;

  return -1;
}

THING *
pack_list_element(int8_t i)
{
  if (i < 0)
    return NULL;

  for (THING* ptr = player_pack; ptr != NULL; ptr = ptr->l_next)
    if (i-- == 0)
      return ptr;

  return NULL;
}

int
pack_size(void)
{
  return 22;
}

void
pack_set_last_picked_item(THING* ptr)
{
  last_last_picked_item = last_picked_item;
  last_picked_item = ptr;
}

void
pack_reset_last_picked_item(void)
{
  last_picked_item = last_last_picked_item;
}

static size_t
pack_print_evaluate_item(THING* obj)
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

    case WEAPON: case AMMO:
      worth = weap_info[obj->o_which].oi_worth;
      worth *= 3 * (obj->o_hplus + obj->o_dplus) + obj->o_count;
      obj->o_flags |= ISKNOW;
      break;

    case ARMOR:
      worth = armor_value((enum armor_t)obj->o_which);
      worth += (9 - obj->o_arm) * 100;
      worth += (10 * (armor_ac((enum armor_t)obj->o_which) - obj->o_arm));
      obj->o_flags |= ISKNOW;
      break;

    case SCROLL:
      {
        enum scroll_t scroll = (enum scroll_t)obj->o_which;
        worth = scroll_value(scroll);
        worth *= obj->o_count;
        if (!scroll_is_known(scroll))
          worth /= 2;
        scroll_learn(scroll);
      }
      break;

    case POTION:
      worth = potion_info[obj->o_which].oi_worth;
      worth *= obj->o_count;
      op = &potion_info[obj->o_which];
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
      wand_get_worth((enum wand_t)obj->o_which);
      worth += 20 * obj->o_charges;
      if (!(obj->o_flags & ISKNOW))
        worth /= 2;
      obj->o_flags |= ISKNOW;
      wand_set_known((enum wand_t)obj->o_which);
      break;

    case AMULET:
      worth = 1000;
      break;

    default:
      (void)fail("Unknown type: %c(%d)", obj->o_type, obj->o_type);
      assert(0);
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
  msg("moved onto %s", inv_name(buf, obj, true));
}

static void
pack_add_money(int value)
{
  coord const* player_pos = player_get_pos();
  pack_gold += value;

  mvaddcch(player_pos->y, player_pos->x, floor_ch());
  level_set_ch(player_pos->y, player_pos->x,
      (player_get_room()->r_flags & ISGONE)
        ? PASSAGE
        : FLOOR);

  if (value > 0)
    msg("you found %d gold pieces", value);
}

static void
pack_remove_from_floor(THING* obj)
{
  coord const* player_pos = player_get_pos();

  list_detach(&level_items, obj);
  mvaddcch(player_pos->y, player_pos->x, floor_ch());
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
  if (obj->o_type == SCROLL && obj->o_which == S_SCARE && obj->o_flags & ISFOUND)
  {
    list_detach(&level_items, obj);
    mvaddcch(player_pos->y, player_pos->x, floor_ch());
    level_set_ch(player_pos->y, player_pos->x, floor_ch());
    os_remove_thing(&obj);
    msg("the scroll turns to dust as you pick it up");
    return false;
  }

  /* See if we can stack it with something else in the pack */
  if (obj->o_type == POTION || obj->o_type == SCROLL || obj->o_type == FOOD
      || obj->o_type == AMMO)
    for (THING* ptr = player_pack; ptr != NULL; ptr = ptr->l_next)
      if (ptr->o_type == obj->o_type && ptr->o_which == obj->o_which
          && ptr->o_hplus == obj->o_hplus && ptr->o_dplus == obj->o_dplus)
      {
        if (from_floor)
          pack_remove_from_floor(obj);
        ptr->o_count += obj->o_count;
        ptr->o_pos = obj->o_pos;
        os_remove_thing(&obj);
        obj = ptr;
        is_picked_up = true;
        break;
      }

  /* If we cannot stack it, we need to have available space in the pack */
  if (!is_picked_up && pack_count_items() == pack_size())
  {
    msg("there's no room in your pack");
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
    obj->o_packch = pack_char();
    is_picked_up = true;
  }

  /* Add thing to inventory in a sorted way */
  else if (!is_picked_up)
  {
    THING* prev_ptr = NULL;
    THING* ptr = player_pack;

    /* Try to find an object of the same type */
    while (ptr != NULL && ptr->o_type != obj->o_type)
    {
      prev_ptr = ptr;
      ptr = ptr->l_next;
    }

    /* Move to the end of those objects, or stop if found similar item */
    while (ptr != NULL && ptr->o_type == obj->o_type
           && ptr->o_which != obj->o_which)
    {
      prev_ptr = ptr;
      ptr = ptr->l_next;
    }

    /* Make object ready for insertion */
    if (from_floor)
      pack_remove_from_floor(obj);
    obj->o_packch = pack_char();

    /* Add to list */
    if (ptr == NULL && prev_ptr == NULL)
      list_attach(&player_pack, obj);
    else
    {
      obj->l_next = ptr;
      obj->l_prev = prev_ptr;
      if (ptr != NULL)
        ptr->l_prev = obj;
      if (prev_ptr == NULL)
        player_pack = obj;
      else
        prev_ptr->l_next = obj;
    }
  }

  obj->o_flags |= ISFOUND;

  /* If this was the object of something's desire, that monster will
   * get mad and run at the hero.  */
  for (THING* op = monster_list; op != NULL; op = op->l_next)
    if (op->t_dest == &obj->o_pos)
      op->t_dest = player_pos;

  /* Notify the user */
  if (!silent)
  {
    char buf[MAXSTR];
    msg("you now have %s (%c)", inv_name(buf, obj, true), obj->o_packch, true);
  }
  return true;
}

THING*
pack_remove(THING* obj, bool newobj, bool all)
{
  THING* nobj = obj;

  if (obj->o_count > 1 && !all)
  {
    last_picked_item = obj;
    obj->o_count--;
    if (newobj)
    {
      nobj = allocate_new_item();
      *nobj = *obj;
      nobj->l_next = NULL;
      nobj->l_prev = NULL;
      nobj->o_count = 1;
    }
  }
  else
  {
    last_picked_item = NULL;
    pack_used[obj->o_packch - 'a'] = false;
    list_detach(&player_pack, obj);
  }
  return nobj;
}


void
pack_pick_up(THING* obj, bool force)
{
  if (player_is_levitating())
    return;

  switch (obj->o_type)
  {
    case GOLD:
      if (obj != NULL)
      {
        pack_add_money(obj->o_goldval);
        list_detach(&level_items, obj);
        os_remove_thing(&obj);
        player_get_room()->r_goldval = 0;
      }
      return;

    case POTION: case WEAPON: case AMMO: case FOOD: case ARMOR:
    case SCROLL: case AMULET: case RING: case STICK:
      if (force || option_autopickup(obj->o_type))
        pack_add((THING *) NULL, false);
      else
        pack_move_msg(obj);
      return;
  }

  (void)fail("Unknown type: %c(%d)", obj->o_type, obj->o_type);
  assert(0);
}


THING*
pack_find_magic_item(void)
{
  int nobj = 0;

  for (THING* obj = player_pack; obj != NULL; obj = obj->l_next)
    if (is_magic(obj) && rnd(++nobj) == 0)
      return obj;
  return NULL;
}

THING*
pack_get_item(char const* purpose, int type)
{
  if (pack_count_items_of_type(type) < 1)
  {
    msg("You have no item to %s", purpose);
    return NULL;
  }

  pack_print_inventory(type);
  msg("which object do you want to %s? ", purpose);
  char ch = readchar(true);
  mpos = 0;

  pack_clear_inventory();

  if (ch == KEY_ESCAPE || ch == KEY_SPACE)
  {
    clearmsg();
    return NULL;
  }

  for (THING* obj = player_pack; obj != NULL; obj = obj->l_next)
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

int
pack_count_items(void)
{
  return pack_count_items_of_type(0);
}

int
pack_count_items_of_type(int type)
{
  int num = 0;

  for (THING const* list = player_pack; list != NULL; list = list->l_next)
    if (!type || type == list->o_type ||
        (type == PACK_RENAMEABLE && (list->o_type != FOOD && list->o_type != AMULET)))
      ++num;
  return num;
}

bool
pack_contains_amulet(void)
{
  for (THING const* ptr = player_pack; ptr != NULL; ptr = ptr->l_next)
    if (ptr->o_type == AMULET)
      return true;
  return false;
}

bool
pack_contains(THING *item)
{
  for (THING const* ptr = player_pack; ptr != NULL; ptr = ptr->l_next)
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
  msg("--Press any key to continue--");
  readchar(false);
  touchwin(stdscr);
  clearmsg();
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
  for (THING* list = player_pack; list != NULL; list = list->l_next)
  {
    if (!type || type == list->o_type ||
        (type == PACK_RENAMEABLE && (list->o_type != FOOD && list->o_type != AMULET)))
    {
      /* Print out the item and move to next row */
      wmove(invscr, ++num_items, 1);
      wprintw(invscr, "%c) %s", list->o_packch, inv_name(buf, list, false));
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
  for (THING* obj = player_pack; obj != NULL; obj = obj->l_next)
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
  switch(item->o_type)
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

  char buf[MAXSTR];
  if (!pack_add(obj, true))
  {
    coord const* player_pos = player_get_pos();
    list_attach(&level_items, obj);
    level_set_ch(player_pos->y, player_pos->x, (char)obj->o_type);
    int flags = level_get_flags(player_pos->y, player_pos->x);
    flags |= F_DROPPED;
    level_set_flags(player_pos->y, player_pos->x, (char)flags);
    obj->o_pos = *player_pos;
    msg("dropped %s", inv_name(buf, obj, true));
  }
  else if (!quiet_on_success)
    msg("no longer %s %s", doing, inv_name(buf, obj, true));
  return true;
}

bool pack_item_is_cursed(THING const*item){ return item->o_flags & ISCURSED; }
void pack_curse_item(THING *item)         { item->o_flags |= ISCURSED; }
void pack_uncurse_item(THING *item)       { item->o_flags &= ~ISCURSED; }

THING*
pack_find_arrow(void)
{
  for (THING* ptr = player_pack; ptr != NULL; ptr = ptr->l_next)
    if (ptr->o_which == ARROW)
      return ptr;
  return NULL;
}
