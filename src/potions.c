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

#include <curses.h>
#include <ctype.h>

#include "potions.h"
#include "rogue.h"
#include "status_effects.h"

struct obj_info pot_info[NPOTIONS] = {
  /* io_name,      oi_prob, oi_worth, oi_guess, oi_know */
  { "confusion",         7,        5,     NULL, FALSE },
  { "hallucination",     8,        5,     NULL, FALSE },
  { "poison",            8,        5,     NULL, FALSE },
  { "gain strength",    13,      150,     NULL, FALSE },
  { "see invisible",     3,      100,     NULL, FALSE },
  { "healing",          13,      130,     NULL, FALSE },
  { "monster detection", 6,      130,     NULL, FALSE },
  { "magic detection",   6,      105,     NULL, FALSE },
  { "raise level",       2,      250,     NULL, FALSE },
  { "extra healing",     5,      200,     NULL, FALSE },
  { "haste self",        5,      190,     NULL, FALSE },
  { "restore strength", 13,      130,     NULL, FALSE },
  { "blindness",         5,        5,     NULL, FALSE },
  { "levitation",        6,       75,     NULL, FALSE },
};

bool
is_quaffable(THING *thing)
{
  if (thing == NULL)
    return FALSE;
  else if (thing->o_type != POTION)
  {
    msg(terse
        ? "that's undrinkable"
        : "yuk! Why would you want to drink that?");
    return FALSE;
  }
  else
    return TRUE;
}

void
quaff()
{
  THING *obj = get_item("quaff", POTION);
  THING *tp, *mp;
  bool discardit = FALSE;

  /* Make certain that it is somethings that we want to drink */
  if (!is_quaffable(obj))
    return;

  if (obj == cur_weapon)
    cur_weapon = NULL;

  /* Calculate the effect it has on the poor guy. */
  discardit = (bool)(obj->o_count == 1);
  leave_pack(obj, FALSE, FALSE);
  switch (obj->o_which)
  {
    case P_CONFUSE:
      if (!knows_potion(obj->o_which) && !on(player, ISHALU))
        learn_potion(obj->o_which);
      become_confused(false);
    when P_POISON:
      learn_potion(obj->o_which);
      become_poisoned();
    when P_HEALING:
      learn_potion(obj->o_which);
      become_healed();
    when P_STRENGTH:
      learn_potion(obj->o_which);
      become_stronger();
    when P_MFIND:
      learn_potion(obj->o_which);
      become_monster_seeing(false);
    when P_TFIND:
    {
      /* Potion of magic detection.  Show the potions and scrolls */
      bool show = FALSE;
      if (lvl_obj != NULL)
      {
        wclear(hw);
        for (tp = lvl_obj; tp != NULL; tp = next(tp))
        {
          if (is_magic(tp))
          {
            show = TRUE;
            wmove(hw, tp->o_pos.y, tp->o_pos.x);
            waddcch(hw, MAGIC);
            learn_potion(obj->o_which);
          }
        }
        for (mp = mlist; mp != NULL; mp = next(mp))
        {
          for (tp = mp->t_pack; tp != NULL; tp = next(tp))
          {
            if (is_magic(tp))
            {
              show = TRUE;
              wmove(hw, mp->t_pos.y, mp->t_pos.x);
              waddcch(hw, MAGIC);
            }
          }
        }
      }
      if (show)
      {
        learn_potion(obj->o_which);
        show_win("You sense the presence of magic on this level.--More--");
      }
      else
        msg("you have a %s feeling for a moment, then it passes",
            choose_str("normal", "strange"));
    }
    when P_LSD:
      learn_potion(obj->o_which);
      become_tripping(false);
    when P_SEEINVIS:
      become_true_seeing(false);
    when P_RAISE:
      learn_potion(obj->o_which);
      raise_level();
    when P_XHEAL:
      learn_potion(obj->o_which);
      become_extra_healed();
    when P_HASTE:
      learn_potion(obj->o_which);
      become_hasted(false);
    when P_RESTORE:
      become_restored();
    when P_BLIND:
      learn_potion(obj->o_which);
      become_blind(false);
    when P_LEVIT:
      learn_potion(obj->o_which);
      become_levitating(false);
    otherwise:
      if (wizard)
      {
        msg("what an odd tasting potion!");
        return;
      }
  }
  status();
  /*
   * Throw the item away
   */

  call_it(&pot_info[obj->o_which]);

  if (discardit)
    discard(obj);
  return;
}

void
learn_potion(enum potion_t potion)
{
  pot_info[potion].oi_know = TRUE;
}

bool
knows_potion(enum potion_t potion)
{
  return pot_info[potion].oi_know;
}
