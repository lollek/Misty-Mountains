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

#include "io.h"
#include "pack.h"
#include "fight.h"
#include "list.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "os.h"
#include "things.h"
#include "state.h"
#include "options.h"
#include "monster.h"
#include "rogue.h"

#include "weapons.h"


struct obj_info weap_info[MAXWEAPONS +1];
int group = 2;

static THING *last_wielded_weapon = NULL;

#define NO_WEAPON '\0'

static struct init_weaps {
    char *iw_dam;	/* Damage when wielded */
    char *iw_hrl;	/* Damage when thrown */
    char iw_launch;	/* Launching weapon */
    int iw_flags;	/* Miscellaneous flags */
} init_dam[MAXWEAPONS] = {
    { "2x4",	"1x3",	NO_WEAPON,	0,		},	/* Mace */
    { "3x4",	"1x2",	NO_WEAPON,	0,		},	/* Long sword */
    { "1x1",	"2x3",	NO_WEAPON,	0,		},	/* Bow */
    { "1x1",	"2x3",	BOW,		ISMANY|ISMISL,	},	/* Arrow */
    { "1x6",	"1x4",	NO_WEAPON,	ISMISL|ISMISL,	},	/* Dagger */
    { "4x4",	"1x2",	NO_WEAPON,	0,		},	/* 2h sword */
    { "1x1",	"1x3",	NO_WEAPON,	ISMANY|ISMISL,	},	/* Dart */
    { "1x2",	"2x4",	NO_WEAPON,	ISMANY|ISMISL,	},	/* Shuriken */
    { "2x3",	"1x6",	NO_WEAPON,	ISMISL,		},	/* Spear */
};

struct obj_info weap_info[] = {
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
  int8_t i = pack_list_index(last_wielded_weapon);

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

bool
missile(int ydelta, int xdelta)
{
  THING *obj = pack_get_item("throw", WEAPON);
  THING *monster_at_pos;
  THING *weapon = pack_equipped_item(EQUIPMENT_RHAND);

  if (obj == NULL)
    return false;

  if (obj->o_type == ARMOR)
  {
    msg("you can't throw armor");
    return missile(ydelta, xdelta);
  }

  obj = pack_remove(obj, true, false);
  do_motion(obj, ydelta, xdelta);
  monster_at_pos = level_get_monster(obj->o_pos.y, obj->o_pos.x);

  /* Throwing an arrow without a bow always misses */
  if (obj->o_which == ARROW && (weapon == NULL || weapon->o_which != BOW))
  {
    if (monster_at_pos && !to_death)
      fight_missile_miss(obj, set_mname(monster_at_pos));
    fall(obj, true);
    return true;
  }
  /* AHA! Here it has hit something.  If it is a wall or a door,
   * or if it misses (combat) the monster, put it on the floor */
  else if (monster_at_pos == NULL ||
      !hit_monster(obj->o_pos.y, obj->o_pos.x, obj))
    fall(obj, true);

  return true;
}

void
do_motion(THING *obj, int ydelta, int xdelta)
{
  coord *player_pos = player_get_pos();
  int ch;

  /* Come fly with us ... */
  obj->o_pos = *player_pos;
  for (;;)
  {
    /* Erase the old one */
    if (!same_coords(&obj->o_pos, player_pos) &&
        cansee(obj->o_pos.y, obj->o_pos.x) && !terse)
    {
      ch = level_get_ch(obj->o_pos.y, obj->o_pos.x);
      if (ch == FLOOR && !show_floor())
        ch = SHADOW;
      mvaddcch(obj->o_pos.y, obj->o_pos.x, (chtype)ch);
    }

    /* Get the new position */
    obj->o_pos.y += ydelta;
    obj->o_pos.x += xdelta;
    if (step_ok(ch = level_get_type(obj->o_pos.y, obj->o_pos.x))
       && ch != DOOR)
    {
      /* It hasn't hit anything yet, so display it if it alright. */
      if (cansee(obj->o_pos.y, obj->o_pos.x) && !terse)
      {
        usleep(10000);
        mvaddcch(obj->o_pos.y, obj->o_pos.x, (chtype)obj->o_type);
        refresh();
      }
      continue;
    }
    break;
  }
}

void
fall(THING *obj, bool pr)
{
  PLACE *pp;
  static coord fpos;

  if (fallpos(&obj->o_pos, &fpos))
  {
    pp = INDEX(fpos.y, fpos.x);
    pp->p_ch = (char) obj->o_type;
    obj->o_pos = fpos;
    if (cansee(fpos.y, fpos.x))
    {
      if (pp->p_monst != NULL)
        pp->p_monst->t_oldch = (char) obj->o_type;
      else
        mvaddcch(fpos.y, fpos.x, (chtype)obj->o_type);
    }
    list_attach(&lvl_obj, obj);
    return;
  }

  if (pr)
    msg("the %s vanishes as it hits the ground",
        weap_info[obj->o_which].oi_name);
  _discard(&obj);
}

void
init_weapon(THING *weap, int which)
{
  struct init_weaps *iwp;

  weap->o_type = WEAPON;
  weap->o_which = which;
  iwp = &init_dam[which];
  strncpy(weap->o_damage, iwp->iw_dam, sizeof(weap->o_damage));
  strncpy(weap->o_hurldmg,iwp->iw_hrl, sizeof(weap->o_hurldmg));
  weap->o_launch = iwp->iw_launch;
  weap->o_flags = iwp->iw_flags;
  weap->o_hplus = 0;
  weap->o_dplus = 0;
  weap->o_arm = 0;

  if (which == SPEAR)
    weap->o_arm = 2;

  if (which == DAGGER)
  {
    weap->o_count = rnd(4) + 2;
    weap->o_group = group++;
  }
  else if (weap->o_flags & ISMANY)
  {
    weap->o_count = rnd(8) + 8;
    weap->o_group = group++;
  }
  else
  {
    weap->o_count = 1;
    weap->o_group = 0;
  }
}

int
hit_monster(int y, int x, THING *obj)
{
  static coord mp;

  assert(obj != NULL);

  mp.y = y;
  mp.x = x;
  return fight_against_monster(&mp, obj, true);
}

char *
num(int n1, int n2, char type)
{
  static char numbuf[10];

  sprintf(numbuf, n1 < 0 ? "%d" : "+%d", n1);
  if (type == WEAPON)
    sprintf(&numbuf[strlen(numbuf)], n2 < 0 ? ",%d" : ",+%d", n2);
  return numbuf;
}

bool
weapon_wield(THING *weapon)
{
  THING *currently_wielding = pack_equipped_item(EQUIPMENT_RHAND);

  if (currently_wielding != NULL)
    if (!pack_unequip(EQUIPMENT_RHAND, true))
      return true;

  pack_remove(weapon, false, true);
  pack_equip_item(weapon);

  msg("wielding %s", inv_name(weapon, true));
  last_wielded_weapon = currently_wielding;
  return true;
}

void
set_last_weapon(THING *weapon)
{
  last_wielded_weapon = weapon;
}

bool
last_weapon(void)
{
  if (last_wielded_weapon == NULL)
  {
    msg("you have no weapon to switch to");
    return false;
  }

  if (!pack_contains(last_wielded_weapon))
  {
    last_wielded_weapon = NULL;
    msg("you have no weapon to switch to");
    return false;
  }

  return weapon_wield(last_wielded_weapon);
}

bool
fallpos(coord *pos, coord *newpos)
{
  int y, x, cnt, ch;

  cnt = 0;
  for (y = pos->y - 1; y <= pos->y + 1; y++)
    for (x = pos->x - 1; x <= pos->x + 1; x++)
    {
      coord *player_pos = player_get_pos();
      /*
       * check to make certain the spot is empty, if it is,
       * put the object there, set it in the level list
       * and re-draw the room if he can see it
       */
      if (y == player_pos->y && x == player_pos->x)
        continue;
      if (((ch = level_get_ch(y, x)) == FLOOR || ch == PASSAGE)
          && rnd(++cnt) == 0)
      {
        newpos->y = y;
        newpos->x = x;
      }
    }
  return cnt != 0;
}
