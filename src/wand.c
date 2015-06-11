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

#include <string.h>
#include <assert.h>

#include "io.h"
#include "pack.h"
#include "list.h"
#include "monster.h"
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
#include "fight.h"
#include "magic.h"
#include "rogue.h"

#include "wand.h"

#define NMATERIAL ((sizeof(material)) / sizeof(*material))
static char const* material[] = {
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

static char const* _wand_material[MAXSTICKS];

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

void* __wands_ptr(void) { return wands; }

void wand_init(void)
{
  bool used[sizeof(material) / sizeof(*material)];

  assert (NMATERIAL >= MAXSTICKS);

  for (size_t i = 0; i < NMATERIAL; i++)
    used[i] = false;

  for (size_t i = 0; i < MAXSTICKS; i++)
  {
    int j = os_rand_range(NMATERIAL);

    while (used[j])
      j = os_rand_range(NMATERIAL);

    _wand_material[i] = material[j];
    used[j] = true;
  }
}

bool wand_save_state(void)
{
  assert(MAXSTICKS > 0);
  assert(NMATERIAL > 0);
  assert(MAXSTICKS <= NMATERIAL);

  /* Save material */
  for (int i = 0; i < MAXSTICKS; i++)
    if (state_save_index(material, NMATERIAL, _wand_material[i]))
      return 1;

  /* Save obj_info data */
  state_save_obj_info(wands, MAXSTICKS);
  return 0;
}

bool wand_load_state(void)
{
  assert(MAXSTICKS > 0);
  assert(NMATERIAL > 0);
  assert(MAXSTICKS <= NMATERIAL);

  /* Load material */
  for (int i = 0; i < MAXSTICKS; i++)
    if (state_load_index(material, NMATERIAL, &_wand_material[i]))
      return 1;

  /* Load obj_info data */
  state_load_obj_info(wands, MAXSTICKS);
  return 0;

}

char const*
wand_material(enum wand_t wand)
{
  assert(wand >= 0 && wand < MAXSTICKS);
  return _wand_material[wand];
}

THING*
wand_create(int wand)
{
  THING* new_wand = os_calloc_thing();

  memset(new_wand->o.o_damage, 0, sizeof(new_wand->o.o_damage));
  new_wand->o.o_damage[0] = (struct damage){1, 1};

  memset(new_wand->o.o_hurldmg, 0, sizeof(new_wand->o.o_hurldmg));
  new_wand->o.o_hurldmg[0] = (struct damage){1, 1};

  new_wand->o.o_arm = 11;
  new_wand->o.o_count = 1;

  new_wand->o.o_type = STICK;
  if (wand < 0 || wand >= MAXSTICKS)
    new_wand->o.o_which = (int)pick_one(wands, MAXSTICKS);
  else
    new_wand->o.o_which = wand;

  switch (new_wand->o.o_which)
  {
    case WS_LIGHT: new_wand->o.o_charges = os_rand_range(10) + 10; break;
    default:       new_wand->o.o_charges = os_rand_range(5) + 3;   break;
  }

  return new_wand;
}

/* This can only be used inside the wand_zap() function */
static THING*
wand_find_target(int* y, int* x, int dy, int dx)
{
  *y = player_y();
  *x = player_x();

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
  THING* drainee[40];
  coord* player_pos = player_get_pos();

  /* First cnt how many things we need to spread the hit points among */
  struct room *corp = level_get_ch(player_pos->y, player_pos->x) == DOOR
    ? &passages[level_get_flags(player_pos->y, player_pos->x) & F_PNUM]
    : NULL;
  bool inpass = player_get_room()->r_flags & ISGONE;
  THING** dp = drainee;

  for (THING* mp = monster_list; mp != NULL; mp = mp->t.l_next)
    if (mp->t.t_room == player_get_room()
        || mp->t.t_room == corp
        ||(inpass && level_get_ch(mp->t.t_pos.y, mp->t.t_pos.x) == DOOR &&
          &passages[level_get_flags(mp->t.t_pos.y, mp->t.t_pos.x) & F_PNUM]
          == player_get_room()))
      *dp++ = mp;

  int cnt = (int)(dp - drainee);
  if (cnt == 0)
  {
    msg("you have a tingling feeling");
    return;
  }

  *dp = NULL;
  player_lose_health(player_get_health() / 2);
  msg("You feel an intense pain");
  cnt = player_get_health() / cnt;

  /* Now zot all of the monsters */
  for (dp = drainee; *dp; dp++)
  {
    THING* mp = *dp;
    mp->t.t_stats.s_hpt -= cnt;
    if (mp->t.t_stats.s_hpt <= 0)
      monster_on_death(mp, monster_seen_by_player(mp));
    else
    {
      monster_start_running(&mp->t.t_pos);
      char buf[MAXSTR];
      msg("%s screams in pain", monster_name(mp, buf));
    }
  }
}

static void
wand_spell_polymorph(THING* target)
{
  assert(target != NULL);

  coord pos = {
    .y = target->t.t_pos.y,
    .x = target->t.t_pos.x
  };

  if (target->t.t_type == 'F')
    player_remove_held();

  THING* target_pack = target->t.t_pack;
  list_detach(&monster_list, target);

  bool was_seen = monster_seen_by_player(target);
  if (was_seen)
  {
    mvaddcch(pos.y, pos.x, level_get_ch(pos.y, pos.x));
    char buf[MAXSTR];
    addmsg("%s", monster_name(target, buf));
  }

  char oldch = target->t.t_oldch;

  char monster = (char)(os_rand_range(26) + 'A');
  bool same_monster = monster == target->t.t_type;

  monster_new(target, monster, &pos);
  if (monster_seen_by_player(target))
  {
    mvaddcch(pos.y, pos.x, monster);
    if (same_monster)
      msg(" now looks a bit different");
    else
    {
      char buf[MAXSTR];
      msg(" turned into a %s", monster_name(target, buf));
    }
  }
  else if (was_seen)
    msg(" disappeared");

  target->t.t_oldch = oldch;
  target->t.t_pack = target_pack;
  wands[WS_POLYMORPH].oi_know |= monster_seen_by_player(target);
}

static void
wand_spell_cancel(THING* target)
{
  assert(target != NULL);

  if (target->t.t_type == 'F')
    player_remove_held();

  monster_set_cancelled(target);
  monster_remove_invisible(target);
  monster_remove_confusing(target);

  target->t.t_disguise = target->t.t_type;
  if (monster_seen_by_player(target))
    mvaddcch(target->t.t_pos.y, target->t.t_pos.x, target->t.t_disguise);
}

static void
wand_spell_magic_missile(int dy, int dx)
{
  THING bolt;
  memset(&bolt, 0, sizeof(bolt));
  bolt.o.o_type = '*';
  bolt.o.o_hplus = 100;
  bolt.o.o_dplus = 1;
  bolt.o.o_flags = ISMISL;
  bolt.o.o_damage[0] = (struct damage){0, 0};
  bolt.o.o_hurldmg[0] = (struct damage){1, 4};

  THING* weapon = pack_equipped_item(EQUIPMENT_RHAND);
  if (weapon != NULL)
    bolt.o.o_launch = weapon->o.o_which;

  io_missile_motion(&bolt, dy, dx);

  THING* target = level_get_monster(bolt.o.o_pos.y, bolt.o.o_pos.x);
  if (target == NULL)
    msg("the missle vanishes with a puff of smoke");
  else if (monster_save_throw(VS_MAGIC, target))
  {
    char buf[MAXSTR];
    msg("the missle missed the %s", monster_name(target, buf));
  }
  else
    fight_against_monster(&bolt.o.o_pos, &bolt, true);
}



bool
wand_zap(void)
{
  coord const* dir = get_dir();
  if (dir == NULL)
    return false;
  coord delta = *dir;

  THING* obj = pack_get_item("zap with", STICK);
  if (obj == NULL)
    return false;
  else if (obj->o.o_type != STICK)
  {
    msg("you can't zap with that!");
    return false;
  }
  else if (obj->o.o_charges == 0)
  {
    msg("nothing happens");
    return true;
  }

  assert(obj->o.o_which >= 0 && obj->o.o_which < MAXSTICKS);

  THING* tp;
  int y;
  int x;
  switch (obj->o.o_which)
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
        new_pos.y = y - delta.y;
        new_pos.x = x - delta.x;

        tp->t.t_dest = player_get_pos();
        tp->t.t_flags |= ISRUN;

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
            tp->t.t_flags &= ~ISSLOW;
          else
            tp->t.t_flags |= ISHASTE;
          monster_start_running(&c);
          char buf[MAXSTR];
          msg("%s became faster", monster_name(tp, buf));
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
            tp->t.t_flags &= ~ISHASTE;
          else
            tp->t.t_flags |= ISSLOW;
          tp->t.t_turn = true;
          monster_start_running(&c);
          char buf[MAXSTR];
          msg("%s became slower", monster_name(tp, buf));
        }
        else
          msg("You did not hit anything");
      }
      break;

    case WS_ELECT:
      wands[WS_ELECT].oi_know = true;
      magic_bolt(player_get_pos(), &delta, "bolt");
      break;

    case WS_FIRE:
      wands[WS_FIRE].oi_know = true;
      magic_bolt(player_get_pos(), &delta, "flame");
      break;

    case WS_COLD:
      wands[WS_COLD].oi_know = true;
      magic_bolt(player_get_pos(), &delta, "ice");
      break;

    case WS_NOP:
      msg("You are usure if anything happened");
      break;

    default:
      msg("what a bizarre schtick!");
      break;
    }

    obj->o.o_charges--;
    return true;
}

char*
wand_description(THING* obj, char* buf)
{
  char* ptr = buf;
  struct obj_info oi = wands[obj->o.o_which];

  if (oi.oi_know || oi.oi_guess)
  {
    if (obj->o.o_count == 1)
      strcpy(ptr, "A wand");
    else
      sprintf(ptr, "%d wands", obj->o.o_count);

    ptr += strlen(ptr);
    if (oi.oi_know)
      sprintf(ptr, " of %s", oi.oi_name);
    else if (oi.oi_guess)
      sprintf(ptr, " called %s", oi.oi_guess);

    ptr += strlen(ptr);
    if (obj->o.o_flags & ISKNOW)
      sprintf(ptr, " [%d charges]", obj->o.o_charges);

    ptr += strlen(ptr);
    sprintf(ptr, " (%s)", wand_material((enum wand_t)obj->o.o_which));
  }
  else if (obj->o.o_count == 1)
    sprintf(ptr, "A %s wand", wand_material((enum wand_t)obj->o.o_which));
  else
    sprintf(ptr, "%d %s wands", obj->o.o_count,
            wand_material((enum wand_t)obj->o.o_which));

  return buf;
}

bool wand_is_known(enum wand_t wand)
{
  return wands[wand].oi_know;
}
void wand_set_known(enum wand_t wand)
{
  wands[wand].oi_know = true;
}

void wand_set_name(enum wand_t wand, char const* new_name)
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

int wand_get_worth(enum wand_t wand)
{
  return wands[wand].oi_worth;
}
