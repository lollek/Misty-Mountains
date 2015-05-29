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
    int j = rnd(NMATERIAL);

    while (used[j])
      j = rnd(NMATERIAL);

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
  THING* new_wand = allocate_new_item();

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
    new_wand->o_which = (int)pick_one(wands, MAXSTICKS);
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

  for (THING* mp = mlist; mp != NULL; mp = mp->l_next)
    if (mp->t_room == player_get_room()
        || mp->t_room == corp
        ||(inpass && level_get_ch(mp->t_pos.y, mp->t_pos.x) == DOOR &&
          &passages[level_get_flags(mp->t_pos.y, mp->t_pos.x) & F_PNUM]
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
    mp->t_stats.s_hpt -= cnt;
    if (mp->t_stats.s_hpt <= 0)
      monster_on_death(mp, see_monst(mp));
    else
    {
      monster_start_running(&mp->t_pos);
      msg("%s screams in pain", set_mname(mp));
    }
  }
}

static void
wand_spell_polymorph(THING* target)
{
  assert(target != NULL);

  coord pos = {
    .y = target->t_pos.y,
    .x = target->t_pos.x
  };

  if (target->t_type == 'F')
    player_remove_held();

  THING* target_pack = target->t_pack;
  list_detach(&mlist, target);

  bool was_seen = see_monst(target);
  if (was_seen)
  {
    mvaddcch(pos.y, pos.x, level_get_ch(pos.y, pos.x));
    addmsg("%s", set_mname(target));
  }

  char oldch = target->t_oldch;

  char monster = (char)(rnd(26) + 'A');
  bool same_monster = monster == target->t_type;

  monster_new(target, monster, &pos);
  if (see_monst(target))
  {
    mvaddcch(pos.y, pos.x, monster);
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
wand_spell_cancel(THING* target)
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
  assert(sizeof(bolt.o_hurldmg) >= sizeof("1x4"));
  bolt.o_type = '*';
  bolt.o_hplus = 100;
  bolt.o_dplus = 1;
  bolt.o_flags = ISMISL;
  strcpy(bolt.o_hurldmg,"1x4");

  THING* weapon = pack_equipped_item(EQUIPMENT_RHAND);
  if (weapon != NULL)
    bolt.o_launch = weapon->o_which;

  do_motion(&bolt, dy, dx);

  THING* target = level_get_monster(bolt.o_pos.y, bolt.o_pos.x);
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
  coord const* dir = get_dir();
  if (dir == NULL)
    return false;
  coord delta = *dir;

  THING* obj = pack_get_item("zap with", STICK);
  if (obj == NULL)
    return false;
  else if (obj->o_type != STICK)
  {
    msg("you can't zap with that!");
    return false;
  }
  else if (obj->o_charges == 0)
  {
    msg("nothing happens");
    return true;
  }

  assert(obj->o_which >= 0 && obj->o_which < MAXSTICKS);

  THING* tp;
  int y;
  int x;
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

static bool
fire_bolt_handle_bounces(coord* pos, coord* dir, tile* dirtile, bool retval)
{
  msg("(%d:%d)[%d:%d]", pos->y, pos->x, dir->y, dir->x);
  char ch = level_get_type(pos->y, pos->x);
  if (ch != VWALL && ch != HWALL && ch != SHADOW)
    return retval;

  /* Treat shadow as a wall */
  if (ch == SHADOW)
  {
    if (dir->x != 0 && dir->y == 0)
      ch = VWALL;
    else if (dir->y != 0 && dir->x == 0)
      ch = HWALL;
    else if (dir->y < 0)
    {
      char y_ch = level_get_ch(pos->y + 1, pos->x);
      ch =  (y_ch == SHADOW || y_ch == HWALL || y_ch == HWALL)
        ? VWALL : HWALL;
    }
    else if (dir->y > 0)
    {
      char y_ch = level_get_ch(pos->y - 1, pos->x);
      ch =  (y_ch == SHADOW || y_ch == HWALL || y_ch == HWALL)
        ? VWALL : HWALL;
    }
  }
  assert(ch != SHADOW);

  /* Handle potential bouncing */
  if (ch == VWALL)
  {
    pos->x -= dir->x;
    dir->x = -dir->x;
  }
  else if (ch == HWALL)
  {
    pos->y -= dir->y;
    dir->y = -dir->y;
  }

  if (*dirtile == TILE_BOLT_DIAGDOWN)
    *dirtile = TILE_BOLT_DIAGUP;
  else if (*dirtile == TILE_BOLT_DIAGUP)
    *dirtile = TILE_BOLT_DIAGDOWN;

  /* It's possible for a bolt to bounce directly from one wall to another
   * if you hit a corner, thus, we need to go through everything again. */
  return fire_bolt_handle_bounces(pos, dir, dirtile, true);
}

static void
fire_bolt_hit_player(coord* start, char const* missile_name)
{
  if (!player_save_throw(VS_MAGIC))
  {
    player_lose_health(roll(6, 6));
    if (player_get_health() <= 0)
    {
      if (start == player_get_pos())
        death(missile_name[0]);
      else
        death(level_get_monster(start->y, start->x)->t_type);
    }
    msg("you are hit by the %s", missile_name);
  }
  else
    msg("the %s whizzes by you", missile_name);
}

static void
fire_bolt_hit_monster(THING* mon, coord* start, coord* pos, char* missile_name)
{
  mon->t_oldch = level_get_ch(pos->y, pos->x);
  if (!monster_save_throw(VS_MAGIC, mon))
  {
    THING bolt;
    memset(&bolt, 0, sizeof(bolt));
    bolt.o_type = WEAPON;
    bolt.o_which = FLAME;
    bolt.o_hplus = 100;
    bolt.o_dplus = 0;
    bolt.o_pos = *pos;
    bolt.o_flags |= ISMISL;
    bolt.o_launch = -1;
    assert(sizeof(bolt.o_hurldmg) >= sizeof("6x6"));
    strcpy(bolt.o_hurldmg, "6x6");
    weap_info[FLAME].oi_name = missile_name;

    if (mon->t_type == 'D' && strcmp(missile_name, "flame") == 0)
      msg("the flame bounces off the dragon");
    else
      hit_monster(pos->y, pos->x, &bolt);
  }
  else if (level_get_type(pos->y, pos->x) != 'M' || mon->t_disguise == 'M')
  {
    if (start == player_get_pos())
      monster_start_running(pos);
    else
      msg("the %s whizzes past %s", missile_name, set_mname(mon));
  }
}

void
fire_bolt(coord* start, coord* dir, char* name)
{
  tile dirtile = TILE_ERROR;
  switch (dir->y + dir->x)
  {
    case 0: dirtile = TILE_BOLT_DIAGUP; break;
    case 1: case -1:
      dirtile = (dir->y == 0
          ? TILE_BOLT_HORIZONTAL
          : TILE_BOLT_VERTICAL);
      break;
    case 2: case -2: dirtile = TILE_BOLT_DIAGDOWN; break;
  }
  assert (dirtile != TILE_ERROR);

  enum attribute bolt_type = ATTR_FIRE;

  if (!strcmp(name, "ice"))
    bolt_type = ATTR_ICE;
  else if (!strcmp(name, "flame"))
    bolt_type = ATTR_FIRE;

  coord pos = *start;
  struct charcoord {
    int y;
    int x;
    char ch;
  } spotpos[BOLT_LENGTH];

  /* Special case when someone is standing in a doorway and aims at the wall
   * nearby OR when stainding in a passage and aims at the wall
   * Note that both of those things are really stupid to do */
  char starting_pos = level_get_ch(start->y, start->x);
  if (starting_pos == DOOR || starting_pos == PASSAGE)
  {
    char first_bounce = level_get_ch(start->y + dir->y, start->x + dir->x);
    bool is_player = same_coords(start, player_get_pos());
    if (first_bounce == HWALL || first_bounce == VWALL || first_bounce == SHADOW)
    {
      if (is_player)
        for (int i = 0; i < BOLT_LENGTH; ++i)
          fire_bolt_hit_player(start, name);
      else
        for (int i = 0; i < BOLT_LENGTH; ++i)
          fire_bolt_hit_monster(level_get_monster(start->y, start->x),
                                start, start, name);
      return;
    }
  }

  int i;
  for (i = 0; i < BOLT_LENGTH; ++i)
  {
    pos.y += dir->y;
    pos.x += dir->x;

    if (fire_bolt_handle_bounces(&pos, dir, &dirtile, false))
      msg("the %s bounces", name);

    /* Handle potential hits */
    if (same_coords(&pos, player_get_pos()))
      fire_bolt_hit_player(start, name);

    THING* tp = level_get_monster(pos.y, pos.x);
    if (tp != NULL)
      fire_bolt_hit_monster(tp, start, &pos, name);

    spotpos[i].x = pos.x;
    spotpos[i].y = pos.y;
    spotpos[i].ch = mvincch(pos.y, pos.x);
    io_addch(dirtile, bolt_type);
    refresh();
  }

  usleep(200000);

  for (int j = i -1; j >= 0; --j)
    mvaddcch(spotpos[j].y, spotpos[j].x, spotpos[j].ch);
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
    sprintf(ptr, " (%s)", wand_material((enum wand_t)obj->o_which));
  }
  else if (obj->o_count == 1)
    sprintf(ptr, "A %s wand", wand_material((enum wand_t)obj->o_which));
  else
    sprintf(ptr, "%d %s wands", obj->o_count,
            wand_material((enum wand_t)obj->o_which));

  return buf;
}

const char *wand_nickname(THING *obj)
{
  return wands[obj->o_which].oi_guess;
}

bool wand_is_known(enum wand_t wand)
{
  return wands[wand].oi_know;
}
void wand_set_known(enum wand_t wand)
{
  wands[wand].oi_know = true;
}

void wand_set_name(enum wand_t wand, const char *new_name)
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
