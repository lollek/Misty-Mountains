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
  int ac = thing->t_stats.s_arm;

  if (is_player)
  {
    THING *arm = equipped_item(EQUIPMENT_ARMOR);
    THING *weapon = equipped_item(EQUIPMENT_RHAND);

    ac  = arm ? arm->o_arm : ac;
    ac -= weapon ? weapon->o_arm : 0;
    ac -= ISRING(LEFT, R_PROTECT) ? cur_ring[LEFT]->o_arm : 0;
    ac -= ISRING(RIGHT, R_PROTECT) ? cur_ring[RIGHT]->o_arm : 0;
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
    take_off();
  if (equipped_item(EQUIPMENT_ARMOR) != NULL)
    return true;

  waste_time();
  leave_pack(obj, false, true);
  equip_item(obj);

  if (!terse)
    addmsg("you are now ");
  msg("wearing %s", inv_name(obj, true, true));
  return true;
}

void
rust_players_armor(void)
{
  THING *arm = equipped_item(EQUIPMENT_ARMOR);
  if (arm == NULL || arm->o_type != ARMOR || arm->o_which == LEATHER ||
      arm->o_arm >= 9)
    return;

  if ((arm->o_flags & ISPROT) || ISWEARING(R_SUSTARM))
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

bool
take_off(void)
{
  THING *obj = equipped_item(EQUIPMENT_ARMOR);

  if (obj == NULL)
  {
    msg(terse
        ? "not wearing armor"
        : "you aren't wearing any armor");
    return false;
  }

  if (obj->o_flags & ISCURSED)
  {
    msg("you can't. Your armor appears to be cursed");
    return true;
  }

  waste_time();
  if (!add_pack(obj, true))
  {
    attach(lvl_obj, obj);
    chat(hero.y, hero.x) = (char) obj->o_type;
    flat(hero.y, hero.x) |= F_DROPPED;
    obj->o_pos = hero;
    msg("dropped %s", inv_name(obj, true, true));
    return true;
  }

  unequip_item(EQUIPMENT_ARMOR);

  addmsg(terse
      ? "was"
      : "you used to be");
  msg(" wearing %s", inv_name(obj, true, true));

  return true;
}
