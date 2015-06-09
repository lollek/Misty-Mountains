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

void
do_motion(THING* obj, int ydelta, int xdelta)
{
  coord* player_pos = player_get_pos();
  int ch;

  /* Come fly with us ... */
  obj->o_pos = *player_pos;
  for (;;)
  {
    /* Erase the old one */
    if (!coord_same(&obj->o_pos, player_pos) &&
        cansee(obj->o_pos.y, obj->o_pos.x))
    {
      ch = level_get_ch(obj->o_pos.y, obj->o_pos.x);
      mvaddcch(obj->o_pos.y, obj->o_pos.x, (chtype)ch);
    }

    /* Get the new position */
    obj->o_pos.y += ydelta;
    obj->o_pos.x += xdelta;
    if (step_ok(ch = level_get_type(obj->o_pos.y, obj->o_pos.x))
       && ch != DOOR)
    {
      /* It hasn't hit anything yet, so display it if it alright. */
      if (cansee(obj->o_pos.y, obj->o_pos.x))
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
fall(THING* obj, bool pr)
{
  coord fpos;
  if (fallpos(&obj->o_pos, &fpos))
  {
    PLACE* pp = level_get_place(fpos.y, fpos.x);
    pp->p_ch = (char) obj->o_type;
    obj->o_pos = fpos;
    if (cansee(fpos.y, fpos.x))
    {
      if (pp->p_monst != NULL)
        pp->p_monst->t_oldch = (char) obj->o_type;
      else
        mvaddcch(fpos.y, fpos.x, (chtype)obj->o_type);
    }
    list_attach(&level_items, obj);
    return;
  }

  if (pr)
    msg("the %s vanishes as it hits the ground",
        weap_info[obj->o_which].oi_name);
  _discard(&obj);
}

THING*
weapon_create(int which, bool random_stats)
{
  THING* weap = allocate_new_item();
  memset(weap, 0, sizeof(*weap));

  if (which == -1)
    which = (int)pick_one(weap_info, MAXWEAPONS);

  weap->o_type  = WEAPON;
  weap->o_which = which;

  struct init_weaps* iwp = &init_dam[which];
  weap->o_launch     = iwp->iw_launch;
  weap->o_flags      = iwp->iw_flags;
  weap->o_damage[0]  = iwp->iw_dam;
  weap->o_hurldmg[0] = iwp->iw_hrl;

  if (weap->o_flags & ISMANY)
    weap->o_type = AMMO;

  if (which == SPEAR)
    weap->o_arm = 2;

  if (which == DAGGER)
    weap->o_count = rnd(4) + 2;
  else if (weap->o_flags & ISMANY)
    weap->o_count = rnd(8) + 8;
  else
    weap->o_count = 1;

  if (random_stats)
  {
    int rand = rnd(100);
    if (rand < 10)
    {
      weap->o_flags |= ISCURSED;
      weap->o_hplus -= rnd(3) + 1;
    }
    else if (rand < 15)
      weap->o_hplus += rnd(3) + 1;
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
  msg("wielding %s", inv_name(buf, weapon, true));
  last_wielded_weapon = currently_wielding;
  return true;
}

void
set_last_weapon(THING* weapon)
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
fallpos(coord const* pos, coord* newpos)
{
  int cnt = 0;
  for (int y = pos->y - 1; y <= pos->y + 1; y++)
    for (int x = pos->x - 1; x <= pos->x + 1; x++)
    {
      coord *player_pos = player_get_pos();
      /*
       * check to make certain the spot is empty, if it is,
       * put the object there, set it in the level list
       * and re-draw the room if he can see it
       */
      if (y == player_pos->y && x == player_pos->x)
        continue;

      int ch = level_get_ch(y, x);
      if ((ch == FLOOR || ch == PASSAGE) && rnd(++cnt) == 0)
      {
        newpos->y = y;
        newpos->x = x;
      }
    }
  return cnt != 0;
}

void
weapon_description(THING* obj, char* buf)
{
  char* ptr = buf;
  char const* obj_name = weap_info[obj->o_which].oi_name;

  if (obj->o_count == 1)
    ptr += sprintf(ptr, "A%s %s", vowelstr(obj_name), obj_name);
  else
    ptr += sprintf(ptr, "%d %ss", obj->o_count, obj_name);

  if (obj->o_type == AMMO || obj->o_which == BOW)
  {
    for (int i = 0; i < MAXATTACKS; ++i)
      if (obj->o_hurldmg[i].sides != 0 && obj->o_hurldmg[i].dices != 0)
        ptr += sprintf(ptr, "%s%dd%d", i == 0 ? " (" : "/",
            obj->o_hurldmg[i].sides,
            obj->o_hurldmg[i].dices);
    strcpy(ptr++, ")");
  }
  else if (obj->o_type == WEAPON)
  {
    for (int i = 0; i < MAXATTACKS; ++i)
      if (obj->o_damage[i].sides != 0 && obj->o_damage[i].dices != 0)
        ptr += sprintf(ptr, "%s%dd%d", i == 0 ? " (" : "/",
            obj->o_damage[i].sides,
            obj->o_damage[i].dices);
    strcpy(ptr++, ")");
  }
  else
  {
    (void)fail("Bad type: %p->o_type == %d\r\n", obj, obj->o_type);
    assert(0);
  }

  if (obj->o_flags & ISKNOW)
  {
    ptr += sprintf(ptr, " (");
    ptr += sprintf(ptr, obj->o_hplus < 0 ? "%d," : "+%d,", obj->o_hplus);
    ptr += sprintf(ptr, obj->o_dplus < 0 ? "%d)" : "+%d)", obj->o_dplus);
  }

  if (obj->o_arm != 0)
    ptr += sprintf(ptr, obj->o_arm < 0 ? " [%d]" : " [+%d]", obj->o_arm);

  if (obj->o_label != NULL)
    ptr += sprintf(ptr, " called %s", obj->o_label);

}

