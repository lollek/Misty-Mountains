/*
 * Routines dealing specifically with rings
 *
 * @(#)rings.c	4.19 (Berkeley) 05/29/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include "io.h"
#include "pack.h"
#include "daemons.h"
#include "misc.h"
#include "player.h"
#include "weapons.h"
#include "rogue.h"

#include "rings.h"

struct obj_info ring_info[] = {
  { "protection",		 9, 400, NULL, false },
  { "add strength",		 9, 400, NULL, false },
  { "sustain strength",		 5, 280, NULL, false },
  { "searching",		10, 420, NULL, false },
  { "see invisible",		10, 310, NULL, false },
  { "adornment",		 1,  10, NULL, false },
  { "aggravate monster",	10,  10, NULL, false },
  { "dexterity",		 8, 440, NULL, false },
  { "increase damage",		 8, 400, NULL, false },
  { "regeneration",		 4, 460, NULL, false },
  { "slow digestion",		 9, 240, NULL, false },
  { "teleportation",		 5,  30, NULL, false },
  { "stealth",			 7, 470, NULL, false },
  { "maintain armor",		 5, 380, NULL, false },
};

bool
ring_put_on(void)
{
  THING *obj = pack_get_item("put on", RING);

  /* Make certain that it is somethings that we want to wear */
  if (obj == NULL)
    return false;

  if (obj->o_type != RING)
  {
    msg(terse
      ? "not a ring"
      : "it would be difficult to wrap that around a finger");
    return ring_put_on();
  }

  /* Try to put it on */
  if (!pack_equip_item(obj))
  {
    msg(terse
      ? "wearing two"
      : "you already have a ring on each hand");
    return false;
  }
  pack_remove(obj, false, true);

  /* Calculate the effect it has on the poor guy. */
  switch (obj->o_which)
  {
    case R_ADDSTR: player_modify_strength(obj->o_arm); break;
    case R_SEEINVIS: invis_on(); break;
    case R_AGGR: aggravate(); break;
    }

  if (!terse)
    addmsg("you are now wearing ");
  msg("%s", inv_name(obj, true));
  return true;
}

bool
ring_take_off(void)
{
  enum equipment_pos ring;
  THING *obj;

  /* Try right, then left */
  if (pack_equipped_item(EQUIPMENT_RRING) != NULL)
    ring = EQUIPMENT_RRING;
  else
    ring = EQUIPMENT_LRING;

  obj = pack_equipped_item(ring);

  if (!pack_unequip(ring, false))
    return false;

  switch (obj->o_which)
  {
    case R_ADDSTR:
      player_modify_strength(-obj->o_arm);
      break;
    case R_SEEINVIS:
      player_remove_true_sight();
      daemon_extinguish_fuse(player_remove_true_sight);
      break;
  }

  return true;
}

int
ring_drain_amount(void)
{
  int total_eat = 0;
  int uses[] = {
    1, /* R_PROTECT */  1, /* R_ADDSTR */
    1, /* R_SUSTSTR */ -3, /* R_SEARCH */
   -5, /* R_SEEINVIS */ 0, /* R_NOP */
    0, /* R_AGGR */    -3, /* R_ADDHIT */
   -3, /* R_ADDDAM */   2, /* R_REGEN */
   -2, /* R_DIGEST */   0, /* R_TELEPORT */
    1, /* R_STEALTH */  1  /* R_SUSTARM */
  };
  int i;

  for (i = 0; i < RING_SLOTS_SIZE; ++i)
  {
    THING *ring = pack_equipped_item(ring_slots[i]);
    if (ring != NULL)
    {
      int eat = uses[ring->o_which];
      if (eat < 0)
        eat = rnd(-eat) == 0;
      if (ring->o_which == R_DIGEST)
        eat = -eat;
      total_eat += eat;
    }
  }
  return total_eat;
}

char *
ring_bonus(THING *obj)
{
  static char buf[10];

  if (!(obj->o_flags & ISKNOW))
    return "";

  switch (obj->o_which)
  {
    case R_PROTECT: case R_ADDSTR: case R_ADDDAM: case R_ADDHIT:
      sprintf(buf, " [%s]", num(obj->o_arm, 0, RING));
      break;
    default:
      return "";
  }
  return buf;
}
