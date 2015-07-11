/*
 * Functions for dealing with problems brought about by weapons
 *
 * @(#)weapons.c	4.34 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "fight.h"
#include "io.h"
#include "item.h"
#include "level.h"
#include "list.h"
#include "misc.h"
#include "monster.h"
#include "options.h"
#include "os.h"
#include "pack.h"
#include "player.h"
#include "rogue.h"
#include "state.h"
#include "things.h"

#include "weapons.h"

struct obj_info weapon_info[MAXWEAPONS +1];

static THING* last_wielded_weapon = NULL;

#define NO_WEAPON '\0'

static struct init_weaps {
    struct damage const iw_dam;		/* Damage when wielded */
    struct damage const iw_hrl;		/* Damage when thrown */
    char                iw_launch;	/* Launching weapon */
    int                 iw_flags;	/* Miscellaneous flags */
} init_dam[MAXWEAPONS] = {
    { {2,4}, {1,3}, NO_WEAPON,  0,             },	/* Mace */
    { {3,4}, {1,2}, NO_WEAPON,  0,             },	/* Long sword */
    { {1,1}, {2,3}, NO_WEAPON,  0,             },	/* Bow */
    { {0,0}, {2,3}, BOW,        ISMANY|ISMISL, },	/* Arrow */
    { {1,6}, {1,4}, NO_WEAPON,  ISMISL,        },	/* Dagger */
    { {4,4}, {1,2}, NO_WEAPON,  0,             },	/* 2h sword */
    { {0,0}, {1,3}, NO_WEAPON,  ISMANY|ISMISL, },	/* Dart */
    { {0,0}, {2,4}, NO_WEAPON,  ISMANY|ISMISL, },	/* Shuriken */
    { {2,3}, {1,6}, NO_WEAPON,  ISMISL,        },	/* Spear */
};

struct obj_info weapon_info[] = {
    { "mace",				11,   8, NULL, false },
    { "long sword",			11,  15, NULL, false },
    { "short bow",			12,  15, NULL, false },
    { "arrow",				12,   1, NULL, false },
    { "dagger",				 8,   3, NULL, false },
    { "two handed sword",		10,  75, NULL, false },
    { "dart",				12,   2, NULL, false },
    { "shuriken",			12,   5, NULL, false },
    { "spear",				12,   5, NULL, false },
    /* DO NOT REMOVE: fake entry for dragon's breath */
    { NULL,				0,    0, NULL, false },	
};

bool weapons_save_state(void)
{
  int8_t i = pack_list_index(&last_wielded_weapon->o);

  assert(i >= -1);
  assert(i < pack_size());

  return state_save_int8(i);
}

bool weapons_load_state(void)
{
  int8_t i = 0;
  bool status = state_load_int8(&i);

  assert(i >= -1);
  assert(i < pack_size());

  last_wielded_weapon = pack_list_element(i);

  return status;
}

void
weapon_missile_fall(THING* obj, bool pr)
{
  coord fpos;
  if (fallpos(&obj->o.o_pos, &fpos))
  {
    PLACE* pp = level_get_place(fpos.y, fpos.x);
    pp->p_ch = (char) obj->o.o_type;
    obj->o.o_pos = fpos;
    if (cansee(fpos.y, fpos.x))
    {
      if (pp->p_monst != NULL)
        pp->p_monst->t.t_oldch = (char) obj->o.o_type;
      else
        mvaddcch(fpos.y, fpos.x, (chtype)obj->o.o_type);
    }
    list_attach(&level_items, obj);
    return;
  }

  if (pr)
    io_msg("the %s vanishes as it hits the ground",
        weapon_info[obj->o.o_which].oi_name);
  os_remove_thing(&obj);
}

THING*
weapon_create(int which, bool random_stats)
{
  if (which == -1)
    which = (int)pick_one(weapon_info, MAXWEAPONS);

  THING* weap = os_calloc_thing();
  weap->o.o_type  = WEAPON;
  weap->o.o_which = which;

  struct init_weaps* iwp = &init_dam[which];
  weap->o.o_launch     = iwp->iw_launch;
  weap->o.o_flags      = iwp->iw_flags;
  weap->o.o_damage     = iwp->iw_dam;
  weap->o.o_hurldmg    = iwp->iw_hrl;

  if (weap->o.o_flags & ISMANY)
    weap->o.o_type = AMMO;

  if (which == SPEAR)
    weap->o.o_arm = 2;

  if (which == DAGGER)
    weap->o.o_count = os_rand_range(4) + 2;
  else if (weap->o.o_flags & ISMANY)
    weap->o.o_count = os_rand_range(8) + 8;
  else
    weap->o.o_count = 1;

  if (random_stats)
  {
    int rand = os_rand_range(100);
    if (rand < 10)
    {
      weap->o.o_flags |= ISCURSED;
      weap->o.o_hplus -= os_rand_range(3) + 1;
    }
    else if (rand < 15)
      weap->o.o_hplus += os_rand_range(3) + 1;
  }

  return weap;
}

bool
weapon_wield(THING* weapon)
{
  THING* currently_wielding = pack_equipped_item(EQUIPMENT_RHAND);
  if (currently_wielding != NULL)
    if (!pack_unequip(EQUIPMENT_RHAND, true))
      return true;

  pack_remove(weapon, false, true);
  pack_equip_item(weapon);

  char buf[MAXSTR];
  io_msg("wielding %s", inv_name(buf, weapon, true));
  last_wielded_weapon = currently_wielding;
  return true;
}

void
weapon_set_last_used(THING* weapon)
{
  last_wielded_weapon = weapon;
}

bool
weapon_wield_last_used(void)
{
  if (last_wielded_weapon == NULL || !pack_contains(last_wielded_weapon))
  {
    last_wielded_weapon = NULL;
    io_msg("you have no weapon to switch to");
    return false;
  }

  return weapon_wield(last_wielded_weapon);
}

void
weapon_description(item const* item, char* buf)
{
  char* ptr = buf;
  char const* obj_name = weapon_info[item_subtype(item)].oi_name;

  if (item_count(item) == 1)
    ptr += sprintf(ptr, "A%s %s", vowelstr(obj_name), obj_name);
  else
    ptr += sprintf(ptr, "%d %ss", item_count(item), obj_name);

  if (item_type(item) == AMMO || item_subtype(item) == BOW)
  {
    int dices = item_throw_damage(item)->dices;
    int sides = item_throw_damage(item)->sides;
    ptr += sprintf(ptr, " (%dd%d)", sides, dices);
  }
  else if (item_type(item) == WEAPON)
  {
    int dices = item_damage(item)->dices;
    int sides = item_damage(item)->sides;
    ptr += sprintf(ptr, " (%dd%d)", sides, dices);
  }
  else
  {
    io_debug("Bad type: %p->o_type == %d\r\n", item, item_type(item));
    assert(0);
  }

  if (item_is_known(item))
  {
    int p_hit = item_bonus_hit(item);
    int p_dmg = item_bonus_damage(item);
    ptr += sprintf(ptr, " (");
    ptr += sprintf(ptr, p_hit < 0 ? "%d," : "+%d,", p_hit);
    ptr += sprintf(ptr, p_dmg < 0 ? "%d)" : "+%d)", p_dmg);
  }

  if (item_armor(item) != 0)
  {
    int p_armor = item_armor(item);
    ptr += sprintf(ptr, p_armor < 0 ? " [%d]" : " [+%d]", p_armor);
  }

  if (item_nickname(item) != NULL)
    ptr += sprintf(ptr, " called %s", item_nickname(item));

}

