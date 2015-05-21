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

#include <stdbool.h>

#include "io.h"
#include "pack.h"
#include "list.h"
#include "level.h"
#include "misc.h"
#include "player.h"
#include "monster.h"
#include "state.h"
#include "colors.h"
#include "rogue.h"

#include "potions.h"

/* Colors of the potions */
static char const* p_colors[NPOTIONS];

struct obj_info pot_info[NPOTIONS] = {
  /* io_name,      oi_prob, oi_worth, oi_guess, oi_know */
  { "confusion",         7,        5,     NULL, false },
  { "hallucination",     8,        5,     NULL, false },
  { "poison",            8,        5,     NULL, false },
  { "gain strength",    13,      150,     NULL, false },
  { "see invisible",     3,      100,     NULL, false },
  { "healing",          13,      130,     NULL, false },
  { "monster detection", 6,      130,     NULL, false },
  { "magic detection",   6,      105,     NULL, false },
  { "raise level",       2,      250,     NULL, false },
  { "extra healing",     5,      200,     NULL, false },
  { "haste self",        5,      190,     NULL, false },
  { "restore strength", 13,      130,     NULL, false },
  { "blindness",         5,        5,     NULL, false },
  { "levitation",        6,       75,     NULL, false },
};

void
potions_init(void)
{
  bool used[color_max()];

  for (int i = 0; i < color_max(); i++)
    used[i] = false;

  for (int i = 0; i < NPOTIONS; i++)
  {
    int j;
    do
      j = rnd(color_max());
    while (used[j]);
    used[j] = true;
    p_colors[i] = color_get(j);
  }
}


bool
potion_save_state(void)
{
  for (int32_t i = 0; i < NPOTIONS; ++i)
  {
    int32_t j;
    for (j = 0; j < color_max(); ++j)
      if (p_colors[i] == color_get(j))
        break;

    if (state_save_int32(j == color_max() ? -1 : j))
      return fail("potion_save_state() i=%d, j=%d\r\n", i, j);
  }
  return 0;
}

bool potion_load_state(void)
{
  for (int32_t i = 0; i < NPOTIONS; ++i)
  {
    int32_t tmp;
    if (state_load_int32(&tmp) || tmp < -1 || tmp > color_max())
      return fail("potion_load_state() i=%d, tmp=%d\r\n", i, tmp);

    p_colors[i] = i >= 0 ? color_get(tmp) : NULL;
  }
  return 0;
}

/** is_quaffable
 * Can we dring thing? */
static bool
is_quaffable(THING *thing)
{
  if (thing == NULL)
    return false;
  else if (thing->o_type != POTION)
  {
    msg(terse
        ? "that's undrinkable"
        : "yuk! Why would you want to drink that?");
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
  pot_info[potion].oi_know = true;
}

bool
potion_quaff_something(void)
{
  THING *obj = pack_get_item("quaff", POTION);
  THING *tp, *mp;
  bool discardit = false;

  /* Make certain that it is somethings that we want to drink */
  if (!is_quaffable(obj))
    return false;

  /* Calculate the effect it has on the poor guy. */
  discardit = (bool)(obj->o_count == 1);
  pack_remove(obj, false, false);
  switch (obj->o_which)
  {
    case P_CONFUSE:
      if (!player_is_hallucinating())
        potion_learn(obj->o_which);
      player_set_confused(false);
      break;
    case P_POISON:
      potion_learn(obj->o_which);
      player_become_poisoned();
      break;
    case P_HEALING:
      potion_learn(obj->o_which);
      player_restore_health(roll(player_get_level(), 4), true);
      player_remove_blind();
      msg("you begin to feel better");
      break;
    case P_STRENGTH:
      potion_learn(obj->o_which);
      player_modify_strength(1);
      msg("you feel stronger, now.  What bulging muscles!");
      break;
    case P_MFIND:
      potion_learn(obj->o_which);
      player_add_sense_monsters(false);
      break;
    case P_TFIND:
    {
      /* Potion of magic detection.  Show the potions and scrolls */
      bool show = false;
      if (lvl_obj != NULL)
      {
        wclear(hw);
        for (tp = lvl_obj; tp != NULL; tp = tp->l_next)
        {
          if (is_magic(tp))
          {
            show = true;
            wmove(hw, tp->o_pos.y, tp->o_pos.x);
            waddcch(hw, MAGIC);
            potion_learn(obj->o_which);
          }
        }
        for (mp = mlist; mp != NULL; mp = mp->l_next)
        {
          for (tp = mp->t_pack; tp != NULL; tp = tp->l_next)
          {
            if (is_magic(tp))
            {
              show = true;
              wmove(hw, mp->t_pos.y, mp->t_pos.x);
              waddcch(hw, MAGIC);
            }
          }
        }
      }
      if (show)
      {
        potion_learn(obj->o_which);
        show_win("You sense the presence of magic on this level.--More--");
      }
      else
        msg("you have a %s feeling for a moment, then it passes",
            player_is_hallucinating() ? "normal" : "strange");
    }
    break;
    case P_LSD:
      potion_learn(obj->o_which);
      player_set_hallucinating(false);
      break;
    case P_SEEINVIS:
      player_add_true_sight(false);
      break;
    case P_RAISE:
      if (game_type == DEFAULT)
      {
        potion_learn(obj->o_which);
        player_raise_level();
      }
      else if (game_type == QUICK)
      {
        level++;
        level_new();
        msg("you fell through the floor!");
      }
      break;
    case P_XHEAL:
      potion_learn(obj->o_which);
      player_restore_health(roll(player_get_level(), 8), true);
      player_remove_blind();
      player_remove_hallucinating();
      msg("you begin to feel much better");
      break;
    case P_HASTE:
      potion_learn(obj->o_which);
      player_increase_speed(false);
      break;
    case P_RESTORE:
      if (player_strength_is_weakened())
      {
        player_restore_strength();
        msg("you feel your strength returning");
      }
      else
        msg("you feel warm all over");
      break;
    case P_BLIND:
      potion_learn(obj->o_which);
      player_set_blind(false);
      break;
    case P_LEVIT:
      potion_learn(obj->o_which);
      player_start_levitating(false);
      break;
    default:
      msg("what an odd tasting potion!");
      return true;
  }
  status();

  /* Throw the item away */

  call_it("potion", &pot_info[obj->o_which]);

  if (discardit)
    _discard(&obj);
  return true;
}

void
potion_description(THING const* obj, char buf[])
{
  struct obj_info *op = &pot_info[obj->o_which];
  if (op->oi_know)
  {
    if (obj->o_count == 1)
      buf += sprintf(buf, "A potion of %s", op->oi_name);
    else
      buf += sprintf(buf, "%d potions of %s", obj->o_count, op->oi_name);
  }
  else
  {
    if (obj->o_count == 1)
      buf += sprintf(buf, "A %s potion", p_colors[obj->o_which]);
    else
      buf += sprintf(buf, "%d %s potions", obj->o_count, p_colors[obj->o_which]);
  }
  if (op->oi_guess)
    buf += sprintf(buf, " called %s", op->oi_guess);
}

