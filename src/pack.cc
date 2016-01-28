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

#include <string>
#include <list>

using namespace std;

#include "error_handling.h"
#include "coordinate.h"
#include "game.h"
#include "scrolls.h"
#include "io.h"
#include "armor.h"
#include "potions.h"
#include "rings.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "weapons.h"
#include "wand.h"
#include "things.h"
#include "monster.h"
#include "os.h"
#include "level_rooms.h"
#include "options.h"
#include "rogue.h"
#include "item.h"

#include "pack.h"

int const PACK_RENAMEABLE = -1;

int                  pack_gold = 0;
static list<Item*> player_pack;

static struct equipment_t
{
  Item* ptr;
  string const description;
} equipment[NEQUIPMENT] = {
  { nullptr, "Body" },
  { nullptr, "Right Hand" },
  { nullptr, "Right Ring" },
  { nullptr, "Left Ring" }
};

/* Is the character used in the pack? */
static bool pack_used[26];

enum equipment_pos pack_ring_slots[PACK_RING_SLOTS] = {
  EQUIPMENT_RRING,
  EQUIPMENT_LRING
};

int
pack_size(void)
{
  return 22;
}

static size_t
pack_print_evaluate_item(Item* item)
{
  int worth = 0;
  if (item == nullptr)
    return 0;

  switch (item->o_type)
  {
    case FOOD:
      worth = 2 * item->o_count;
      break;

    case WEAPON: case AMMO:
      worth = static_cast<int>(weapon_info[static_cast<size_t>(item->o_which)].oi_worth);
      worth *= 3 * (item->o_hplus + item->o_dplus) + item->o_count;
      item->o_flags |= ISKNOW;
      break;

    case ARMOR:
      worth = Armor::value(static_cast<Armor::Type>(item->o_which));
      worth += (9 - item->o_arm) * 100;
      worth += (10 * (Armor::ac(static_cast<Armor::Type>(item->o_which)) - item->o_arm));
      item->o_flags |= ISKNOW;
      break;

    case SCROLL:
      {
        scroll_t scroll = static_cast<scroll_t>(item->o_which);
        worth = static_cast<int>(scroll_value(scroll));
        worth *= item->o_count;
        if (!scroll_is_known(scroll))
          worth /= 2;
        scroll_learn(scroll);
      }
      break;

    case POTION: {
      Potion::Type subtype = static_cast<Potion::Type>(item->o_which);
      worth = Potion::worth(subtype);
      worth *= item->o_count;
      if (!Potion::is_known(subtype)) {
        worth /= 2;
      }
      Potion::set_known(subtype);
    } break;

    case RING: {
      Ring::Type subtype = static_cast<Ring::Type>(item->o_which);
      worth = Ring::worth(subtype);
      if (subtype == Ring::Type::ADDSTR || subtype == Ring::Type::ADDDAM ||
          subtype == Ring::Type::PROTECT || subtype == Ring::Type::ADDHIT) {
        if (item->o_arm > 0) {
          worth += item->o_arm * 100;
        } else {
          worth = 10;
        }
      }
      if (Ring::is_known(subtype)) {
        worth /= 2;
      }
      Ring::set_known(subtype);
    } break;

    case STICK:
      Wand::worth(static_cast<Wand::Type>(item->o_which));
      worth += 20 * item->o_charges;
      if (!(item->o_flags & ISKNOW))
        worth /= 2;
      Wand::set_known(static_cast<Wand::Type>(item->o_which));
      break;

    case AMULET:
      worth = 1000;
      break;

    default:
      io_debug_fatal("Unknown type: %c(%d)", item->o_type, item->o_type);
      break;
  }

  if (worth < 0)
    worth = 0;

  printw("%5d  %s\n", worth,
      inv_name(item, false).c_str());

  return static_cast<unsigned>(worth);
}

static char
pack_char(void)
{
  bool* bp;
  for (bp = pack_used; *bp; bp++)
    ;
  *bp = true;
  return static_cast<char>(bp - pack_used) + 'a';
}

void
pack_move_msg(Item* obj)
{
  io_msg("moved onto %s", inv_name(obj, true).c_str());
}

static void
pack_add_money(int value)
{
  pack_gold += value;

  Game::io->print_color(player->get_position().x, player->get_position().y, floor_ch());
  Game::level->set_ch(player->get_position(),
      (player->get_room()->r_flags & ISGONE)
        ? PASSAGE
        : FLOOR);

  if (value > 0)
    io_msg("you found %d gold pieces", value);
}

static void
pack_remove_from_floor(Item* obj)
{
  Game::level->items.remove(obj);
  Game::io->print_color(player->get_position().x, player->get_position().y, floor_ch());
  Game::level->set_ch(player->get_position(),
      (player->get_room()->r_flags & ISGONE)
        ? PASSAGE
        : FLOOR);
}

bool
pack_add(Item* obj, bool silent)
{
  bool from_floor = false;

  /* Either obj is an item or we try to take something from the floor */
  if (obj == nullptr)
  {
    obj = Game::level->get_item(player->get_position());
    if (obj == nullptr) {
      error("Item not found on floor");
    }
    from_floor = true;
  }

  /* Check for and deal with scare monster scrolls */
  if (obj->o_type == SCROLL && obj->o_which == S_SCARE && obj->o_flags & ISFOUND)
  {
    Game::level->items.remove(obj);
    Game::io->print_color(player->get_position().x, player->get_position().y, floor_ch());
    Game::level->set_ch(player->get_position(), floor_ch());
    delete obj;
    io_msg("the scroll turns to dust as you pick it up");
    return false;
  }

  /* See if we can stack it with something else in the pack */
  bool is_picked_up = false;
  if (obj->o_type == POTION || obj->o_type == SCROLL || obj->o_type == FOOD
      || obj->o_type == AMMO)
    for (Item* ptr : player_pack) {
      if (ptr->o_type == obj->o_type && ptr->o_which == obj->o_which
          && ptr->o_hplus == obj->o_hplus && ptr->o_dplus == obj->o_dplus)
      {
        if (from_floor)
          pack_remove_from_floor(obj);
        ptr->o_count += obj->o_count;
        ptr->set_pos(obj->get_pos());
        delete obj;
        obj = ptr;
        is_picked_up = true;
        break;
      }
    }

  /* If we cannot stack it, we need to have available space in the pack */
  if (!is_picked_up && pack_count_items() == pack_size())
  {
    io_msg("there's no room in your pack");
    if (from_floor)
      pack_move_msg(obj);
    return false;
  }

  /* Otherwise, just insert it */
  if (!is_picked_up)
  {
    if (from_floor)
      pack_remove_from_floor(obj);
    player_pack.push_back(obj);
    obj->o_packch = pack_char();
    is_picked_up = true;
  }

  obj->o_flags |= ISFOUND;


  /* Notify the user */
  if (!silent) {
    io_msg("you now have %s (%c)", inv_name(obj, true).c_str(), obj->o_packch, true);
  }
  return true;
}

Item* pack_remove(Item* obj, bool newobj, bool all) {
  Item* return_value = obj;

  /* If there are several, we need to alloate a new item to hold it */
  if (obj->o_count > 1 && !all) {
    obj->o_count--;
    if (newobj) {
      return_value = obj->clone();
      return_value->o_count = 1;
    }

  /* Only one item? Just pop and return it */
  } else {
    pack_used[obj->o_packch - 'a'] = false;
    player_pack.remove(obj);
  }
  return return_value;
}


void
pack_pick_up(Item* obj, bool force)
{
  if (player->is_levitating())
    return;

  // If this was the object of something's desire, that monster will
  // get mad and run at the hero.
  monster_aggro_all_which_desire_item(obj);

  switch (obj->o_type)
  {
    case GOLD:
      if (obj != nullptr)
      {
        pack_add_money(obj->o_goldval);
        Game::level->items.remove(obj);
        delete obj;
        obj = nullptr;
        player->get_room()->r_goldval = 0;
      }
      return;

    case POTION: case WEAPON: case AMMO: case FOOD: case ARMOR:
    case SCROLL: case AMULET: case RING: case STICK:
      if (force || option_autopickup(obj->o_type))
        pack_add(nullptr, false);
      else
        pack_move_msg(obj);
      return;
  }

  io_debug("Unknown type: %c(%d)", obj->o_type, obj->o_type);
  assert(0);
}


Item*
pack_find_magic_item(void)
{
  int nobj = 0;

  for (Item* obj : player_pack) {
    if (obj->is_magic() && os_rand_range(++nobj) == 0) {
      return obj;
    }
  }
  return nullptr;
}

Item*
pack_get_item(std::string const& purpose, int type)
{
  if (pack_count_items_of_type(type) < 1)
  {
    io_msg("You have no item to %s", purpose.c_str());
    return nullptr;
  }

  pack_print_inventory(type);
  io_msg("which object do you want to %s? ", purpose.c_str());
  char ch = io_readchar(true);
  io_msg_clear();

  pack_clear_inventory();

  if (ch == KEY_ESCAPE || ch == KEY_SPACE)
  {
    io_msg_clear();
    return nullptr;
  }

  for (Item* obj : player_pack) {
    if (obj->o_packch == ch) {
      return obj;
    }
  }

  io_msg("'%s' is not a valid item",unctrl(static_cast<chtype>(ch)));
  return nullptr;
}

bool
pack_is_empty(void)
{
  return player_pack.empty();
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

  for (Item const* list : player_pack) {
    if (!type || type == list->o_type ||
        (type == PACK_RENAMEABLE && (list->o_type != FOOD && list->o_type != AMULET))) {
      ++num;
    }
  }
  return num;
}

bool
pack_contains_amulet(void)
{
  return find_if(player_pack.cbegin(), player_pack.cend(),
      [] (Item const* ptr) {
    return ptr->o_type == AMULET;
  }) != player_pack.cend();
}

bool
pack_contains(Item *item)
{
  return find(player_pack.cbegin(), player_pack.cend(), item) != player_pack.cend();
}

bool
pack_print_equipment(void)
{
  WINDOW* equipscr = dupwin(stdscr);

  Coordinate orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  char sym = 'a';
  for (unsigned i = 0; i < NEQUIPMENT; ++i)
  {
    if (equipment[i].ptr != nullptr)
    {
      mvwprintw(equipscr, sym - 'a' + 1, 1, "%c) %s: %s",
                sym, equipment[i].description.c_str(),
                inv_name(equipment[i].ptr, false).c_str());
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
  WINDOW* invscr = dupwin(stdscr);

  Coordinate orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  int num_items = 0;
  /* Print out all items */
  for (Item const* list : player_pack) {
    if (!type || type == list->o_type ||
        (type == PACK_RENAMEABLE && (list->o_type != FOOD && list->o_type != AMULET))) {
      /* Print out the item and move to next row */
      wmove(invscr, ++num_items, 1);
      wprintw(invscr, "%c) %s", list->o_packch, inv_name(list, false).c_str());
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
    value += pack_print_evaluate_item(pack_equipped_item(static_cast<equipment_pos>(i)));

  addstr("\nWorth  Item  [Inventory]\n");
  for (Item* obj : player_pack) {
    value += pack_print_evaluate_item(obj);
  }

  printw("\n%5d  Gold Pieces          ", pack_gold);
  refresh();
  return value;
}

Item*
pack_equipped_item(enum equipment_pos pos)
{
  assert (pos >= 0 && pos < (sizeof equipment / sizeof (*equipment)));
  return equipment[pos].ptr;
}

bool
pack_equip_item(Item* item)
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
      pos = equipment[EQUIPMENT_RRING].ptr == nullptr
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

  Item* obj = pack_equipped_item(pos);
  if (obj == nullptr)
  {
    io_msg("not %s anything!", doing);
    return false;
  }

  if (obj->o_flags & ISCURSED)
  {
    io_msg("you can't. It appears to be cursed");
    return false;
  }

  equipment[pos].ptr = nullptr;

  /* Waste time if armor - since they take a while */
  if (pos == EQUIPMENT_ARMOR)
    player->waste_time(1);

  if (!pack_add(obj, true))
  {
    Game::level->items.push_back(obj);
    Game::level->set_ch(player->get_position(), static_cast<char>(obj->o_type));
    obj->set_pos(player->get_position());
    io_msg("dropped %s", inv_name(obj, true).c_str());
  }
  else if (!quiet_on_success)
    io_msg("no longer %s %s", doing, inv_name(obj, true).c_str());
  return true;
}


Item*
pack_find_arrow(void)
{
  auto results = find_if(player_pack.begin(), player_pack.end(),
      [] (Item *i) {
    return i->o_which == ARROW;
  });

  return results == player_pack.end() ? nullptr : *results;
}

static void pack_identify_item_set_know(Item* item, vector<obj_info>& info) {
  size_t subtype = static_cast<size_t>(item_subtype(item));
  info.at(subtype).oi_know = true;
  item->o_flags |= ISKNOW;

  info[subtype].oi_guess.clear();
}

void
pack_identify_item(void)
{
  if (pack_is_empty())
  {
    io_msg("you don't have anything in your pack to identify");
    return;
  }

  Item* obj = pack_get_item("identify", 0);
  if (obj == nullptr)
    return;

  switch (obj->o_type)
  {
    case SCROLL: pack_identify_item_set_know(obj, scroll_info);  break;
    case POTION: Potion::set_known(static_cast<Potion::Type>(obj->o_which)); break;
    case STICK:  Wand::set_known(static_cast<Wand::Type>(obj->o_which)); break;
    case RING:   Ring::set_known(static_cast<Ring::Type>(obj->o_which)); break;
    case WEAPON: case ARMOR: obj->o_flags |= ISKNOW; break;
    default: break;
  }

  io_msg("%s", inv_name(obj, false).c_str());
}


