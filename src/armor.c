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
#include "pack.h"

#include "armor.h"

int a_class[NARMORS] =  {
 8, /* LEATHER */
 7, /* RING_MAIL */
 7, /* STUDDED_LEATHER */
 6, /* SCALE_MAIL */
 5, /* CHAIN_MAIL */
 4, /* SPLINT_MAIL */
 4, /* BANDED_MAIL */
 3, /* PLATE_MAIL */
};

struct obj_info arm_info[NARMORS] = {
    { "leather armor",           20,     20, NULL, false },
    { "ring mail",               15,     25, NULL, false },
    { "studded leather armor",   15,     20, NULL, false },
    { "scale mail",              13,     30, NULL, false },
    { "chain mail",              12,     75, NULL, false },
    { "splint mail",             10,     80, NULL, false },
    { "banded mail",             10,     90, NULL, false },
    { "plate mail",               5,    150, NULL, false },
};

static void
waste_time(void)
{
  do_daemons(BEFORE);
  do_fuses(BEFORE);
  do_daemons(AFTER);
  do_fuses(AFTER);
}

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
  msg("wearing %s", inv_name(obj, true, true));
  return true;
}

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
  msg(" wearing %c) %s", obj->o_packch, inv_name(obj, true, true));

  return true;
}
