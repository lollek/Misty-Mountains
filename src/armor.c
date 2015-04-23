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

#include <stdlib.h>

#include "rogue.h"
#include "io.h"
#include "pack.h"

#include "armor.h"

static struct armor_info_t armors[NARMORS] = {
 /* name                   ac  prob value known */
 { "leather armor",         8, 20,   20,  false },
 { "ring mail",             7, 15,   25,  false },
 { "studded leather armor", 7, 15,   20,  false },
 { "scale mail",            6, 13,   30,  false },
 { "chain mail",            5, 12,   75,  false },
 { "splint mail",           4, 10,   80,  false },
 { "banded mail",           4, 10,   90,  false },
 { "plate mail",            3,  5,  150,  false },
};
void *__armors_ptr(void) { return armors; }

const char * armor_name(enum armor_t i) { return armors[i].name; }
int armor_ac(enum armor_t i) { return armors[i].ac; }
int armor_value(enum armor_t i) { return armors[i].value; }

int
get_ac(THING *thing)
{
  bool is_player = thing == &player;
  int ac = thing->t_stats.s_arm;

  if (is_player)
  {
    THING *arm = equipped_item(EQUIPMENT_ARMOR);
    THING *weapon = equipped_item(EQUIPMENT_RHAND);
    int i;

    ac  = arm ? arm->o_arm : ac;
    ac -= weapon ? weapon->o_arm : 0;
    for (i = 0; i < RING_SLOTS_SIZE; ++i)
    {
      THING *ring = equipped_item(ring_slots[i]);
      if (ring != NULL && ring->o_which == R_PROTECT)
        ac -= ring->o_arm;
    }
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

  if (obj->o_type != ARMOR)
  {
    msg("you can't wear that");
    return wear();
  }

  if (equipped_item(EQUIPMENT_ARMOR) != NULL)
    if (!unequip_item(EQUIPMENT_ARMOR))
      return true;

  waste_time(1);
  leave_pack(obj, false, true);
  equip_item(obj);

  if (!terse)
    addmsg("you are now ");
  msg("wearing %s", inv_name(obj, true));
  return true;
}

char
random_armor_type(void)
{
  int value = rnd(100);
  int i;
  for (i = 0; i < NARMORS; ++i)
  {
    if (value < armors[i].prob)
      return i;
    else
      value -= armors[i].prob;
  }

  /* Error! Sum of probs was not 100 */
  msg("Error! Sum of probabilities is not 100%");
  pr_spec(ARMOR);
  readchar();
  endwin();
  exit(1);
  return 0;
}

void
rust_players_armor(void)
{
  THING *arm = equipped_item(EQUIPMENT_ARMOR);
  if (arm == NULL || arm->o_type != ARMOR || arm->o_which == LEATHER ||
      arm->o_arm >= 9)
    return;

  if ((arm->o_flags & ISPROT) || player_has_ring_with_ability(R_SUSTARM))
  {
    if (!to_death)
      msg("the rust vanishes instantly");
  }
  else
  {
    arm->o_arm++;
    msg(terse
        ? "your armor weakens"
        : "your armor appears to be weaker now. Oh my!");
  }
}

