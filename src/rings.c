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

#include <ctype.h>
#include <string.h>

#include "io.h"
#include "pack.h"
#include "daemons.h"
#include "misc.h"
#include "player.h"
#include "weapons.h"
#include "things.h"
#include "state.h"
#include "options.h"
#include "os.h"
#include "rogue.h"

#include "rings.h"


static char const* r_stones[NRINGS];
static struct stone {
    char* st_name;
    int st_value;
} stones[] = {
  { "agate",		 25},
  { "alexandrite",	 40},
  { "amethyst",	 50},
  { "carnelian",	 40},
  { "diamond",	300},
  { "emerald",	300},
  { "germanium",	225},
  { "granite",	  5},
  { "garnet",		 50},
  { "jade",		150},
  { "kryptonite",	300},
  { "lapis lazuli",	 50},
  { "moonstone",	 50},
  { "obsidian",	 15},
  { "onyx",		 60},
  { "opal",		200},
  { "pearl",		220},
  { "peridot",	 63},
  { "ruby",		350},
  { "sapphire",	285},
  { "stibotantalite",	200},
  { "tiger eye",	 50},
  { "topaz",		 60},
  { "turquoise",	 70},
  { "taaffeite",	300},
  { "zircon",	 	 80},
};
static int NSTONES = (sizeof(stones) / sizeof(*stones));

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

void
ring_init(void)
{
  for (int i = 0; i < NRINGS; i++)
    for (;;)
    {
      int stone = rnd(NSTONES);

      for (int j = 0; j < i; ++j)
        if (r_stones[j] == stones[stone].st_name)
        {
          stone = -1;
          break;
        }

      if (stone == -1)
        continue;

      r_stones[i] = stones[stone].st_name;
      ring_info[i].oi_worth += stones[stone].st_value;
      break;
    }
}

bool
ring_save_state(void)
{
  for (int i = 0; i < NRINGS; i++)
  {
    int32_t j;
    for (j = 0; j < NSTONES; ++j)
      if (r_stones[i] == stones[j].st_name)
        break;

    if (state_save_int32(j == NSTONES ? -1 : j))
      return fail("ring_save_state(), i=%d,j=%d\r\n", i, j);
  }
  return 0;
}

bool
ring_load_state(void)
{
  for (int i = 0; i < NRINGS; i++)
  {
    int32_t j = 0;
    if (state_load_int32(&j))
      return fail("ring_load_state(), i=%d,j=%d, max=%d\r\n", i, j, NSTONES);
    else if (j >= NSTONES)
      return fail("ring_load_state(), i=%d,j=%d, max=%d, j >= NSTONES\r\n",
                  i, j, NSTONES);
    else if (j < -1)
      return fail("ring_load_state(), i=%d,j=%d, max=%d, j < -1\r\n",
                  i, j, NSTONES);

    r_stones[i] = j >= 0 ? stones[j].st_name : NULL;
  }
  return 0;
}



bool
ring_put_on(void)
{
  THING* obj = pack_get_item("put on", RING);

  /* Make certain that it is somethings that we want to wear */
  if (obj == NULL)
    return false;

  if (obj->o_type != RING)
  {
    msg("not a ring");
    return ring_put_on();
  }

  /* Try to put it on */
  if (!pack_equip_item(obj))
  {
    msg("you already have a ring on each hand");
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

  char buf[MAXSTR];
  ring_description(obj, buf);
  buf[0] = (char)tolower(buf[0]);
  msg("now wearing %s", buf);
  return true;
}

bool
ring_take_off(void)
{
  enum equipment_pos ring;

  /* Try right, then left */
  if (pack_equipped_item(EQUIPMENT_RRING) != NULL)
    ring = EQUIPMENT_RRING;
  else
    ring = EQUIPMENT_LRING;

  THING* obj = pack_equipped_item(ring);

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
    1, /* R_PROTECT */  1, /* R_ADDSTR   */  1, /* R_SUSTSTR  */
    1, /* R_SEARCH  */  1, /* R_SEEINVIS */  0, /* R_NOP      */
    0, /* R_AGGR    */  1, /* R_ADDHIT   */  1, /* R_ADDDAM   */
    2, /* R_REGEN   */ -1, /* R_DIGEST   */  0, /* R_TELEPORT */
    1, /* R_STEALTH */  1, /* R_SUSTARM  */
  };

  for (int i = 0; i < PACK_RING_SLOTS; ++i)
  {
    THING *ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring != NULL)
      total_eat += uses[ring->o_which];
  }

  return total_eat;
}

bool
ring_is_known(enum ring_t ring)
{
  return ring_info[ring].oi_know;
}

void
ring_description(THING const* item, char* buf)
{
  struct obj_info* op = &ring_info[item->o_which];
  buf += sprintf(buf, "%s ring", r_stones[item->o_which]);

  if (op->oi_know)
  {
    buf += sprintf(buf, " of %s", op->oi_name);
    switch (item->o_which)
    {
      case R_PROTECT: case R_ADDSTR: case R_ADDDAM: case R_ADDHIT:
        if (item->o_arm > 0)
          buf += sprintf(buf, " [+%d]", item->o_arm);
        else
          buf += sprintf(buf, " [%d]", item->o_arm);
        break;
      default: break;
    }
  }
  else if (op->oi_guess)
    sprintf(buf, " called %s", op->oi_guess);
}

THING*
ring_create(int which, bool random_stats)
{
  THING* ring = allocate_new_item();
  memset(ring, 0, sizeof(*ring));

  if (which == -1)
    which = (int)pick_one(ring_info, NRINGS);

  ring->o_type = RING;
  ring->o_which = which;

  switch (ring->o_which)
  {
    case R_ADDSTR: case R_PROTECT: case R_ADDHIT: case R_ADDDAM:
      if (random_stats)
      {
        ring->o_arm = rnd(3);
        if (ring->o_arm == 0)
        {
          ring->o_arm = -1;
          ring->o_flags |= ISCURSED;
        }
      }
      else
        ring->o_arm = 1;
      break;

    case R_AGGR: case R_TELEPORT:
      ring->o_flags |= ISCURSED;
      break;
  }

  return ring;
}

