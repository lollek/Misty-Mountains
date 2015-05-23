/*
 * Functions to implement the various sticks one might find
 * while wandering around the dungeon.
 *
 * @(#)sticks.c	4.39 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "io.h"
#include "pack.h"
#include "list.h"
#include "monster.h"
#include "rooms.h"
#include "misc.h"
#include "passages.h"
#include "level.h"
#include "player.h"
#include "weapons.h"
#include "state.h"
#include "things.h"
#include "rip.h"
#include "os.h"
#include "options.h"
#include "rogue.h"

#include "wand.h"

#define NMATERIAL ((sizeof(material)) / sizeof(*material))
static const char *material[] = {
  /* Wood */
  "avocado wood", "balsa", "bamboo", "banyan", "birch", "cedar", "cherry",
  "cinnibar", "cypress", "dogwood", "driftwood", "ebony", "elm", "eucalyptus",
  "fall", "hemlock", "holly", "ironwood", "kukui wood", "mahogany",
  "manzanita", "maple", "oaken", "persimmon wood", "pecan", "pine", "poplar",
  "redwood", "rosewood", "spruce", "teak", "walnut", "zebrawood",

  /* Metal */
  "aluminum", "beryllium", "bone", "brass", "bronze", "copper", "electrum",
  "gold", "iron", "lead", "magnesium", "mercury", "nickel", "pewter",
  "platinum", "steel", "silver", "silicon", "tin", "titanium", "tungsten",
  "zinc",
};

static const char *_wand_material[MAXSTICKS];

static struct obj_info wands[] = {
    { "light",			12, 250, NULL, false },
    { "invisibility",		 6,   5, NULL, false },
    { "lightning",		 3, 330, NULL, false },
    { "fire",			 3, 330, NULL, false },
    { "cold",			 3, 330, NULL, false },
    { "polymorph",		15, 310, NULL, false },
    { "magic missile",		10, 170, NULL, false },
    { "haste monster",		10,   5, NULL, false },
    { "slow monster",		11, 350, NULL, false },
    { "drain life",		 9, 300, NULL, false },
    { "nothing",		 1,   5, NULL, false },
    { "teleport away",		 6, 340, NULL, false },
    { "teleport to",		 6,  50, NULL, false },
    { "cancellation",		 5, 280, NULL, false },
};

void *__wands_ptr(void) { return wands; }

void wand_init(void)
{
  size_t i;
  bool used[sizeof(material) / sizeof(*material)];

  assert (NMATERIAL >= MAXSTICKS);

  for (i = 0; i < NMATERIAL; i++)
    used[i] = false;

  for (i = 0; i < MAXSTICKS; i++)
  {
    size_t j = rnd(NMATERIAL);

    while (used[j])
      j = rnd(NMATERIAL);

    _wand_material[i] = material[j];
    used[j] = true;
  }
}

bool wand_save_state(void)
{
  int i;

  assert(MAXSTICKS > 0);
  assert(NMATERIAL > 0);
  assert(MAXSTICKS <= NMATERIAL);

  /* Save material */
  for (i = 0; i < MAXSTICKS; i++)
    if (state_save_index(material, NMATERIAL, _wand_material[i]))
      return 1;

  /* Save obj_info data */
  state_save_obj_info(wands, MAXSTICKS);
  return 0;
}

bool wand_load_state(void)
{
  int i;

  assert(MAXSTICKS > 0);
  assert(NMATERIAL > 0);
  assert(MAXSTICKS <= NMATERIAL);

  /* Load material */
  for (i = 0; i < MAXSTICKS; i++)
    if (state_load_index(material, NMATERIAL, &_wand_material[i]))
      return 1;

  /* Load obj_info data */
  state_load_obj_info(wands, MAXSTICKS);
  return 0;

}

const char *
wand_material(enum wand wand)
{
  assert(wand >= 0 && wand < MAXSTICKS);
  return _wand_material[wand];
}

THING *
wand_create(int wand)
{
  THING *new_wand = allocate_new_item();

  assert(sizeof("1x1") <= sizeof(new_wand->o_damage));
  assert(sizeof("1x1") <= sizeof(new_wand->o_hurldmg));

  new_wand->o_hplus = 0;
  new_wand->o_dplus = 0;
  strcpy(new_wand->o_damage, "1x1");
  strcpy(new_wand->o_hurldmg,"1x1");
  new_wand->o_arm = 11;
  new_wand->o_count = 1;
  new_wand->o_group = 0;
  new_wand->o_flags = 0;

  new_wand->o_type = STICK;
  if (wand < 0 || wand >= MAXSTICKS)
    new_wand->o_which = pick_one(wands, MAXSTICKS);
  else
    new_wand->o_which = wand;

  switch (new_wand->o_which)
  {
    case WS_LIGHT: new_wand->o_charges = rnd(10) + 10; break;
    default:       new_wand->o_charges = rnd(5) + 3;   break;
  }

  return new_wand;
}

/* This can only be used inside the wand_zap() function */
static THING *
wand_find_target(int *y, int *x, int dy, int dx)
{
  coord *player_pos = player_get_pos();
  *x = player_pos->x;
  *y = player_pos->y;

  /* "walk" in the zap direction until we find a target */
  while (step_ok(level_get_type(*y, *x)))
  {
    *y += dy;
    *x += dx;
  }

  return level_get_monster(*y, *x);
}

static void
wand_spell_light(void)
{
  if (player_get_room()->r_flags & ISGONE)
  {
    msg("the corridor glows and then fades");
    return;
  }

  player_get_room()->r_flags &= ~ISDARK;
  room_enter(player_get_pos());
  msg("the rooms is lit by a shimmering %s light", pick_color("blue"));
}

/* take away 1/2 of hero's hit points, then take it away
 * evenly from the monsters in the room (or next to hero
 * if he is in a passage) */
static void
wand_spell_drain_health(void)
{
    THING *mp;
    struct room *corp;
    THING **dp;
    int cnt;
    bool inpass;
    static THING *drainee[40];
    coord *player_pos = player_get_pos();

    /*
     * First cnt how many things we need to spread the hit points among
     */
    cnt = 0;
    if (chat(player_pos->y, player_pos->x) == DOOR)
	corp = &passages[level_get_flags(player_pos->y, player_pos->x) & F_PNUM];
    else
	corp = NULL;
    inpass = (bool)(player_get_room()->r_flags & ISGONE);
    dp = drainee;
    for (mp = mlist; mp != NULL; mp = mp->l_next)
	if (mp->t_room == player_get_room()
            || mp->t_room == corp
            ||(inpass && chat(mp->t_pos.y, mp->t_pos.x) == DOOR &&
              &passages[level_get_flags(mp->t_pos.y, mp->t_pos.x) & F_PNUM]
              == player_get_room()))
		*dp++ = mp;
    if ((cnt = (int)(dp - drainee)) == 0)
    {
	msg("you have a tingling feeling");
	return;
    }
    *dp = NULL;
    player_lose_health(player_get_health() / 2);
    msg("You feel an intense pain");
    cnt = player_get_health() / cnt;
    /*
     * Now zot all of the monsters
     */
    for (dp = drainee; *dp; dp++)
    {
	mp = *dp;
	if ((mp->t_stats.s_hpt -= cnt) <= 0)
	    monster_on_death(mp, see_monst(mp));
	else
        {
	    monster_start_running(&mp->t_pos);
            msg("%s screams in pain", set_mname(mp));
        }
    }
}

static void
wand_spell_polymorph(THING *target)
{
  THING *target_pack;
  coord pos;
  int x;
  int y;
  char monster;
  char oldch;
  bool was_seen;
  bool same_monster;

  assert(target != NULL);

  x = target->t_pos.x;
  y = target->t_pos.y;

  if (target->t_type == 'F')
    player_remove_held();

  target_pack = target->t_pack;
  list_detach(&mlist, target);
  was_seen = see_monst(target);
  if (was_seen)
  {
    mvaddcch(y, x, chat(y, x));
    addmsg("%s", set_mname(target));
  }
  oldch = target->t_oldch;

  pos.y = y;
  pos.x = x;
  monster = rnd(26) + 'A';
  same_monster = monster == target->t_type;

  monster_new(target, monster, &pos);
  if (see_monst(target))
  {
    mvaddcch(y, x, monster);
    if (same_monster)
      msg(" now looks a bit different");
    else
      msg(" turned into a %s", set_mname(target));
  }
  else if (was_seen)
    msg(" disappeared");

  target->t_oldch = oldch;
  target->t_pack = target_pack;
  wands[WS_POLYMORPH].oi_know |= see_monst(target);
}

static void
wand_spell_cancel(THING *target)
{
  assert(target != NULL);

  if (target->t_type == 'F')
    player_remove_held();

  monster_set_cancelled(target);
  monster_remove_invisible(target);
  monster_remove_confusing(target);

  target->t_disguise = target->t_type;
  if (see_monst(target))
    mvaddcch(target->t_pos.y, target->t_pos.x, target->t_disguise);
}

static void
wand_spell_magic_missile(int dy, int dx)
{
  THING bolt;
  THING *target;
  THING *weapon = pack_equipped_item(EQUIPMENT_RHAND);

  assert(sizeof(bolt.o_hurldmg) >= sizeof("1x4"));

  bolt.o_type = '*';
  bolt.o_hplus = 100;
  bolt.o_dplus = 1;
  bolt.o_flags = ISMISL;
  strcpy(bolt.o_hurldmg,"1x4");
  if (weapon != NULL)
    bolt.o_launch = weapon->o_which;

  do_motion(&bolt, dy, dx);
  target = level_get_monster(bolt.o_pos.y, bolt.o_pos.x);
  if (target == NULL)
    msg("the missle vanishes with a puff of smoke");
  else if (monster_save_throw(VS_MAGIC, target))
    msg("the missle missed the %s", set_mname(target));
  else
    hit_monster(bolt.o_pos.y, bolt.o_pos.x, &bolt);
}



bool
wand_zap(void)
{
  const coord *dir = get_dir();
  coord delta;
  THING *obj;
  THING *tp;
  int y;
  int x;

  if (dir == NULL)
    return false;
  delta = *dir;

  obj = pack_get_item("zap with", STICK);
  if (obj == NULL)
    return false;

  if (obj->o_type != STICK)
  {
    msg("you can't zap with that!");
    return false;
  }

  if (obj->o_charges == 0)
  {
    msg("nothing happens");
    return true;
  }

  assert(obj->o_which >= 0 && obj->o_which < MAXSTICKS);

  switch (obj->o_which)
  {
    case WS_LIGHT:
      wands[WS_LIGHT].oi_know = true;
      wand_spell_light();
      break;

    case WS_DRAIN:
      if (player_get_health() > 1)
        wand_spell_drain_health();
      else
      {
        msg("you are too weak to use it");
        return true;
      }
      break;

    case WS_INVIS:
      tp = wand_find_target(&y, &x, delta.y, delta.x);
      if (tp != NULL)
        monster_set_invisible(tp);
      else
        msg("You did not hit anything");
      break;

    case WS_POLYMORPH:
      tp = wand_find_target(&y, &x, delta.y, delta.x);
      if (tp != NULL)
        wand_spell_polymorph(tp);
      else
        msg("You did not hit anything");
      break;

    case WS_CANCEL:
      tp = wand_find_target(&y, &x, delta.y, delta.x);
      if (tp != NULL)
        wand_spell_cancel(tp);
      else
        msg("You did not hit anything");
      break;

    case WS_TELAWAY:
      wands[WS_TELAWAY].oi_know = true;
      player_teleport(NULL);
      break;

    case WS_TELTO:
      wands[WS_TELTO].oi_know = true;
      tp = wand_find_target(&y, &x, delta.y, delta.x);
      if (tp != NULL)
      {
        coord new_pos;
        new_pos.y = y;
        new_pos.x = x;

        tp->t_dest = player_get_pos();
        tp->t_flags |= ISRUN;

        player_teleport(&new_pos);
      }
      else
        msg("You did not hit anything");
      break;

    case WS_MISSILE:
      wands[WS_MISSILE].oi_know = true;
      wand_spell_magic_missile(delta.y, delta.x);
      break;

    case WS_HASTE_M:
      {
        coord c;
        tp = wand_find_target(&c.y, &c.x, delta.y, delta.x);
        if (tp != NULL)
        {
          if (monster_is_slow(tp))
            tp->t_flags &= ~ISSLOW;
          else
            tp->t_flags |= ISHASTE;
          monster_start_running(&c);
          msg("%s became faster", set_mname(tp));
        }
        else
          msg("You did not hit anything");
      }
      break;

    case WS_SLOW_M:
      {
        coord c;
        tp = wand_find_target(&c.y, &c.x, delta.y, delta.x);
        if (tp != NULL)
        {
          if (monster_is_hasted(tp))
            tp->t_flags &= ~ISHASTE;
          else
            tp->t_flags |= ISSLOW;
          tp->t_turn = true;
          monster_start_running(&c);
          msg("%s became slower", set_mname(tp));
        }
        else
          msg("You did not hit anything");
      }
      break;

    case WS_ELECT:
      wands[WS_ELECT].oi_know = true;
      fire_bolt(player_get_pos(), &delta, "bolt");
      break;

    case WS_FIRE:
      wands[WS_FIRE].oi_know = true;
      fire_bolt(player_get_pos(), &delta, "flame");
      break;

    case WS_COLD:
      wands[WS_COLD].oi_know = true;
      fire_bolt(player_get_pos(), &delta, "ice");
      break;

    case WS_NOP:
      msg("You are usure if anything happened");
      break;

    default:
      msg("what a bizarre schtick!");
      break;
    }

    obj->o_charges--;
    return true;
}



void
fire_bolt(coord *start, coord *dir, char *name)
{
    coord *c1, *c2;
    THING *tp;
    char dirch = 0, ch;
    bool hit_hero, used, changed;
    static coord pos;
    static coord spotpos[BOLT_LENGTH];
    THING bolt;

    bolt.o_type = WEAPON;
    bolt.o_which = FLAME;
    strncpy(bolt.o_hurldmg,"6x6",sizeof(bolt.o_hurldmg));
    bolt.o_hplus = 100;
    bolt.o_dplus = 0;
    weap_info[FLAME].oi_name = name;
    switch (dir->y + dir->x)
    {
	case 0: dirch = '/'; break;
	case 1: case -1: dirch = (dir->y == 0 ? HWALL : VWALL); break;
	case 2: case -2: dirch = '\\'; break;
    }
    pos = *start;
    hit_hero = (bool)(start != player_get_pos());
    used = false;
    changed = false;
    for (c1 = spotpos; c1 <= &spotpos[BOLT_LENGTH-1] && !used; c1++)
    {
	pos.y += dir->y;
	pos.x += dir->x;
	*c1 = pos;
	ch = level_get_type(pos.y, pos.x);
	switch (ch)
	{
	    case DOOR:
		/*
		 * this code is necessary if the hero is on a door
		 * and he fires at the wall the door is in, it would
		 * otherwise loop infinitely
		 */
		if (same_coords(player_get_pos(), &pos))
		    goto def;
		/* FALLTHROUGH */
	    case VWALL: case HWALL: case SHADOW:
		if (!changed)
		    hit_hero = !hit_hero;
		changed = false;
		dir->y = -dir->y;
		dir->x = -dir->x;
		c1--;
		msg("the %s bounces", name);
		break;
	    default:
def:
		if (!hit_hero && (tp = level_get_monster(pos.y, pos.x)) != NULL)
		{
		    hit_hero = true;
		    changed = !changed;
		    tp->t_oldch = chat(pos.y, pos.x);
		    if (!monster_save_throw(VS_MAGIC, tp))
		    {
			bolt.o_pos = pos;
			used = true;
			if (tp->t_type == 'D' && strcmp(name, "flame") == 0)
			    msg("the flame bounces off the dragon");
			else
			    hit_monster(pos.y, pos.x, &bolt);
		    }
		    else if (ch != 'M' || tp->t_disguise == 'M')
		    {
			if (start == player_get_pos())
			    monster_start_running(&pos);
			if (terse)
			    msg("%s misses", name);
			else
			    msg("the %s whizzes past %s", name, set_mname(tp));
		    }
		}
		else if (hit_hero && same_coords(&pos, player_get_pos()))
		{
		    hit_hero = false;
		    changed = !changed;
		    if (!player_save_throw(VS_MAGIC))
		    {
			player_lose_health(roll(6, 6));
			if (player_get_health() <= 0)
			{
			    if (start == player_get_pos())
				death('b');
			    else
                              death(level_get_monster(start->y, start->x)->t_type);
			}
			used = true;
			if (terse)
			    msg("the %s hits", name);
			else
			    msg("you are hit by the %s", name);
		    }
		    else
			msg("the %s whizzes by you", name);
		}
		mvaddcch(pos.y, pos.x, dirch);
		refresh();
	}
    }
    for (c2 = spotpos; c2 < c1; c2++)
	mvaddcch(c2->y, c2->x, chat(c2->y, c2->x));
}

char *
wand_description(THING *obj, char *buf)
{
  char *ptr = buf;
  struct obj_info oi = wands[obj->o_which];

  if (oi.oi_know || oi.oi_guess)
  {
    if (obj->o_count == 1)
      strcpy(ptr, "A wand");
    else
      sprintf(ptr, "%d wands", obj->o_count);

    ptr += strlen(ptr);
    if (oi.oi_know)
      sprintf(ptr, " of %s", oi.oi_name);
    else if (oi.oi_guess)
      sprintf(ptr, " called %s", oi.oi_guess);

    ptr += strlen(ptr);
    if (obj->o_flags & ISKNOW)
      sprintf(ptr, " [%d charges]", obj->o_charges);

    ptr += strlen(ptr);
    sprintf(ptr, " (%s)", wand_material(obj->o_which));
  }
  else if (obj->o_count == 1)
    sprintf(ptr, "A %s wand", wand_material(obj->o_which));
  else
    sprintf(ptr, "%d %s wands", obj->o_count, wand_material(obj->o_which));

  return buf;
}

const char *wand_nickname(THING *obj)
{
  return wands[obj->o_which].oi_guess;
}

bool wand_is_known(enum wand wand)
{
  return wands[wand].oi_know;
}
void wand_set_known(enum wand wand)
{
  wands[wand].oi_know = true;
}

void wand_set_name(enum wand wand, const char *new_name)
{
  size_t len = strlen(new_name);

  if (wands[wand].oi_guess != NULL)
  {
    free(wands[wand].oi_guess);
    wands[wand].oi_guess = NULL;
  }

  if (len > 0)
  {
    wands[wand].oi_guess = malloc(len + 1);
    strcpy(wands[wand].oi_guess, new_name);
  }
}

int wand_get_worth(enum wand wand)
{
  return wands[wand].oi_worth;
}
