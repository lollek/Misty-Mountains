/*
 * Function(s) for dealing with potions
 *
 * @(#)potions.c	4.46 (Berkeley) 06/07/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <vector>
#include <string>

using namespace std;

#include "io.h"
#include "pack.h"
#include "level.h"
#include "misc.h"
#include "player.h"
#include "monster.h"
#include "colors.h"
#include "os.h"
#include "rogue.h"
#include "item.h"

#include "potions.h"

/* Colors of the potions */
static string p_colors[NPOTIONS];

vector<obj_info> potion_info = {
  /* io_name,      oi_prob, oi_worth, oi_guess, oi_know */
  { "confusion",         7,        5,       "", false },
  { "hallucination",     8,        5,       "", false },
  { "poison",            8,        5,       "", false },
  { "gain strength",    13,      150,       "", false },
  { "see invisible",     3,      100,       "", false },
  { "healing",          13,      130,       "", false },
  { "monster detection", 6,      130,       "", false },
  { "magic detection",   6,      105,       "", false },
  { "raise level",       2,      250,       "", false },
  { "extra healing",     5,      200,       "", false },
  { "haste self",        5,      190,       "", false },
  { "restore strength", 13,      130,       "", false },
  { "blindness",         5,        5,       "", false },
  { "levitation",        6,       75,       "", false },
};

void
potions_init(void)
{
  /* Pick a unique color for each potion */
  for (int i = 0; i < NPOTIONS; i++)
    for (;;)
    {
      int color = os_rand_range(color_max());

      for (int j = 0; j < i; ++j)
        if (p_colors[j] == color_get(color))
        {
          color = -1;
          break;
        }

      if (color == -1)
        continue;

      p_colors[i] = color_get(color);
      break;
    }
}

/** is_quaffable
 * Can we dring thing? */
static bool
is_quaffable(item const* item)
{
  if (item == nullptr)
    return false;
  else if (item_type(item) != POTION)
  {
    io_msg("that's undrinkable");
    return false;
  }
  else
    return true;
}

/** potion_learn
 * Hero learn what a potion does */
static void
potion_learn(enum potion_t potion)
{
  potion_info[potion].oi_know = true;
}

bool
potion_quaff_something(void)
{
  item* obj = pack_get_item("quaff", POTION);

  /* Make certain that it is somethings that we want to drink */
  if (!is_quaffable(obj))
    return false;

  /* Calculate the effect it has on the poor guy. */
  bool discardit = obj->o_count == 1;
  pack_remove(obj, false, false);
  switch (obj->o_which)
  {
    case P_CONFUSE:
      if (!player_is_hallucinating())
        potion_learn(P_CONFUSE);
      player_set_confused(false);
      break;

    case P_POISON:
      potion_learn(P_POISON);
      player_become_poisoned();
      break;

    case P_HEALING:
      potion_learn(P_HEALING);
      player_restore_health(roll(player_get_level(), 4), true);
      player_remove_blind(0);
      io_msg("you begin to feel better");
      break;

    case P_STRENGTH:
      potion_learn(P_STRENGTH);
      player_modify_strength(1);
      io_msg("you feel stronger, now.  What bulging muscles!");
      break;

    case P_MFIND:
      potion_learn(P_MFIND);
      player_add_sense_monsters(false);
      break;

    case P_TFIND:
    {
      /* Potion of magic detection.  Show the potions and scrolls */
      bool show = false;
      if (!level_items.empty())
      {
        wclear(hw);
        for (item* item : level_items) {
          if (is_magic(item))
          {
            show = true;
            mvwaddcch(hw, item->o_pos.y, item->o_pos.x, MAGIC);
            potion_learn(P_TFIND);
          }
        }

        if (monster_show_if_magic_inventory())
          show = true;
     }

      if (show)
      {
        potion_learn(P_TFIND);
        show_win("You sense the presence of magic on this level.--More--");

      }
      else
        io_msg("you have a strange feeling for a moment, then it passes");
    }
    break;

    case P_LSD:
      potion_learn(P_LSD);
      player_set_hallucinating(false);
      break;

    case P_SEEINVIS:
      player_add_true_sight(false);
      break;

    case P_RAISE:
      potion_learn(P_RAISE);
      player_raise_level();
      break;

    case P_XHEAL:
      potion_learn(P_XHEAL);
      player_restore_health(roll(player_get_level(), 8), true);
      player_remove_blind(0);
      player_remove_hallucinating(0);
      io_msg("you begin to feel much better");
      break;

    case P_HASTE:
      potion_learn(P_HASTE);
      player_increase_speed(false);
      break;

    case P_RESTORE:
      if (player_strength_is_weakened())
      {
        player_restore_strength();
        io_msg("you feel your strength returning");
      }
      else
        io_msg("you feel warm all over");
      break;

    case P_BLIND:
      potion_learn(P_BLIND);
      player_set_blind(false);
      break;

    case P_LEVIT:
      potion_learn(P_LEVIT);
      player_start_levitating(false);
      break;

    default:
      io_msg("what an odd tasting potion!");
      return true;
  }
  io_refresh_statusline();

  /* Throw the item away */
  call_it("potion", &potion_info.at(static_cast<size_t>(obj->o_which)));

  if (discardit)
    delete obj;
  return true;
}

void
potion_description(item const* item, char buf[])
{
  struct obj_info* op = &potion_info.at(static_cast<size_t>(item_subtype(item)));
  if (op->oi_know)
  {
    if (item_count(item) == 1)
      buf += sprintf(buf, "A potion of %s", op->oi_name.c_str());
    else
      buf += sprintf(buf, "%d potions of %s", item_count(item), op->oi_name.c_str());
  }
  else
  {
    string color = p_colors[item_subtype(item)];

    if (item_count(item) == 1)
      buf += sprintf(buf, "A%s %s potion", vowelstr(color).c_str(), color.c_str());
    else
      buf += sprintf(buf, "%d %s potions", item_count(item), color.c_str());

    if (!op->oi_guess.empty())
      sprintf(buf, " called %s", op->oi_guess.c_str());
  }
}

item*
potion_create(int which)
{
  item* pot = new item();

  if (which == -1)
    which = static_cast<int>(pick_one(potion_info, NPOTIONS));

  pot->o_type       = POTION;
  pot->o_which      = which;
  pot->o_count      = 1;
  pot->o_damage     = {1, 2};
  pot->o_hurldmg    = {1, 2};

  return pot;
}

