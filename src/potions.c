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

#include "status_effects.h"
#include "io.h"
#include "pack.h"
#include "list.h"
#include "new_level.h"
#include "misc.h"
#include "rogue.h"

#include "potions.h"

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
      if (!is_hallucinating(&player))
        potion_learn(obj->o_which);
      become_confused(false);
    when P_POISON:
      potion_learn(obj->o_which);
      become_poisoned();
    when P_HEALING:
      potion_learn(obj->o_which);
      become_healed();
    when P_STRENGTH:
      potion_learn(obj->o_which);
      become_stronger();
    when P_MFIND:
      potion_learn(obj->o_which);
      become_monster_seeing(false);
    when P_TFIND:
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
            is_hallucinating(&player) ? "normal" : "strange");
    }
    when P_LSD:
      potion_learn(obj->o_which);
      become_tripping(false);
    when P_SEEINVIS:
      set_true_seeing(&player, true, false);
    when P_RAISE:
      if (game_type == DEFAULT)
      {
        potion_learn(obj->o_which);
        raise_level();
      }
      else if (game_type == QUICK)
      {
        level++;
        new_level();
        msg("you fell through the floor!");
      }
    when P_XHEAL:
      potion_learn(obj->o_which);
      become_extra_healed();
    when P_HASTE:
      potion_learn(obj->o_which);
      become_hasted(false);
    when P_RESTORE:
      become_restored();
    when P_BLIND:
      potion_learn(obj->o_which);
      become_blind(false);
    when P_LEVIT:
      potion_learn(obj->o_which);
      become_levitating(false);
    otherwise:
      msg("what an odd tasting potion!");
      return true;
  }
  status();

  /* Throw the item away */

  call_it(&pot_info[obj->o_which]);

  if (discardit)
    discard(obj);
  return true;
}

