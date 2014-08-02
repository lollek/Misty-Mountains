/*
 * This file contains misc functions for dealing with armor
 * @(#)armor.c	4.14 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include "rogue.h"
#include "io.h"

int
get_ac(THING *thing)
{
  bool is_player = thing == &player;
  int ac;

  if (is_player)
  {
    ac = cur_armor ? cur_armor->o_arm : pstats.s_arm;
    if (cur_weapon && cur_weapon->o_arm != 0)
      ac -= cur_weapon->o_arm;
    if (ISRING(LEFT, R_PROTECT))
      ac -= cur_ring[LEFT]->o_arm;
    if (ISRING(RIGHT, R_PROTECT))
      ac -= cur_ring[RIGHT]->o_arm;
  }
  else
    ac = thing->t_stats.s_arm;

  return 20 - ac;
}

/** wear:
 * The player wants to wear something, so let him/her put it on.  */
bool
wear(void)
{
  THING *obj = get_item("wear", ARMOR);

  if (obj == NULL)
    return false;

  if (obj == cur_armor)
  {
    msg("that's already in use");
    return wear();
  }

  if (obj->o_type != ARMOR)
  {
    msg("you can't wear that");
    return wear();
  }

  if (cur_armor != NULL)
    take_off();
  if (cur_armor != NULL)
    return true;

  waste_time();
  obj->o_flags |= ISKNOW;
  cur_armor = obj;
  if (!terse)
    addmsg("you are now ");
  msg("wearing %s", inv_name(obj, true));
  return true;
}

/** take_off:
 * Get the armor off of the players back */
bool
take_off(void)
{
  THING *obj = cur_armor;

  if (cur_armor == NULL)
  {
    if (terse)
      msg("not wearing armor");
    else
      msg("you aren't wearing any armor");
    return false;
  }

  if (obj->o_flags & ISCURSED)
  {
    msg("you can't. Your armor appears to be cursed");
    return true;
  }

  waste_time();
  cur_armor = NULL;

  if (terse)
    addmsg("was");
  else
    addmsg("you used to be");
  msg(" wearing %c) %s", obj->o_packch, inv_name(obj, true));

  return true;
}

/** waste_time:
 * Do nothing but let other things happen */
void
waste_time(void)
{
  do_daemons(BEFORE);
  do_fuses(BEFORE);
  do_daemons(AFTER);
  do_fuses(AFTER);
}
