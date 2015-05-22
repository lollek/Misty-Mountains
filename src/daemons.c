/*
 * Contains functions for dealing with things that happen in the
 * future.
 *
 * @(#)daemon.c	4.7 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <assert.h>

#include "io.h"
#include "pack.h"
#include "command.h"
#include "monster.h"
#include "rings.h"
#include "misc.h"
#include "player.h"
#include "list.h"
#include "rip.h"
#include "level.h"
#include "options.h"
#include "rogue.h"

#include "daemons.h"

#define EMPTY 0
#define DAEMON -1
#define MAXDAEMONS 20

static int quiet_rounds = 0;

static struct delayed_action daemons[MAXDAEMONS];

void* __daemons_ptr(void) { return daemons; }

/** daemon_empty_slot:
 * Find an empty slot in the daemon/fuse list */
static struct delayed_action*
daemon_empty_slot(void)
{
  for (int i = 0; i < MAXDAEMONS; ++i)
    if (daemons[i].d_type == EMPTY)
      return &daemons[i];

  msg("DEBUG: Ran out of fuse slots :(");
  return NULL;
}

/** daemon_find_slot:
 * Find a particular slot in the table */
static struct delayed_action*
daemon_find_slot(void (*func)())
{
  for (int i = 0; i < MAXDAEMONS; ++i)
    if (daemons[i].d_type != EMPTY && daemons[i].d_func == func)
      return &daemons[i];

  return NULL;
}

/** daemon_run_all:
 * Run all the daemons that are active with the current flag,
 * passing the argument to the function. */
static void
daemon_run_all(int flag)
{
  for (int i = 0; i < MAXDAEMONS; ++i)
    if (daemons[i].d_type == flag && daemons[i].d_time == DAEMON)
      (*daemons[i].d_func)(daemons[i].d_arg);
}

/** daemon_run_fuses:
 * Decrement counters and start needed fuses */
static void
daemon_run_fuses(int flag)
{
  for (int i = 0; i < MAXDAEMONS; ++i)
    if (daemons[i].d_type == flag && daemons[i].d_time > 0
        && --daemons[i].d_time == 0)
    {
      daemons[i].d_type = EMPTY;
      (*daemons[i].d_func)(daemons[i].d_arg);
    }
}

void daemon_run_before(void)
{
  daemon_run_all(BEFORE);
  daemon_run_fuses(BEFORE);
}

void daemon_run_after(void)
{
  daemon_run_all(AFTER);
  daemon_run_fuses(AFTER);
}


/** daemon_start:
 * Start a daemon, takes a function. */
void
daemon_start(void (*func)(), int arg, int type)
{
  struct delayed_action* dev = daemon_empty_slot();
  if (dev != NULL)
  {
    dev->d_type = type;
    dev->d_func = func;
    dev->d_arg = arg;
    dev->d_time = DAEMON;
  }
}

/** daemon_kill:
 * Remove a daemon from the list */
void
daemon_kill(void (*func)())
{
  struct delayed_action* dev = daemon_find_slot(func);
  if (dev != NULL)
    dev->d_type = EMPTY;
}


/** fuse:
 * Start a fuse to go off in a certain number of turns */
void
daemon_start_fuse(void (*func)(), int arg, int time, int type)
{
  struct delayed_action* wire = daemon_empty_slot();
  if (wire != NULL)
  {
    wire->d_type = type;
    wire->d_func = func;
    wire->d_arg = arg;
    wire->d_time = time;
  }
}

/** daemon_lengthen_fuse:
 * Increase the time until a fuse goes off */
void
daemon_lengthen_fuse(void (*func)(), int xtime)
{
  struct delayed_action* wire = daemon_find_slot(func);
  if (wire != NULL)
    wire->d_time += xtime;
}

/** daemon_extinguish_fuse:
 * Put out a fuse */
void
daemon_extinguish_fuse(void (*func)())
{
  struct delayed_action* wire = daemon_find_slot(func);
  if (wire != NULL)
    wire->d_type = EMPTY;
}


/** daemon_reset_doctor
 * Stop the daemon doctor from healing */
void
daemon_reset_doctor(void)
{
  quiet_rounds = 0;
}

/** daemon_doctor:
 * A healing daemon that restors hit points after rest */
void
daemon_doctor(void)
{
  int ohp = player_get_health();
  if (ohp == player_get_max_health())
    return;

  quiet_rounds++;
  if (player_get_level() < 8)
  {
    if (quiet_rounds + (player_get_level() << 1) > 20)
      player_restore_health(1, false);
  }
  else if (quiet_rounds >= 3)
    player_restore_health(rnd(player_get_level() - 7) + 1, false);

  for (int i = 0; i < RING_SLOTS_SIZE; ++i)
  {
    THING *ring = pack_equipped_item(ring_slots[i]);
    if (ring != NULL && ring->o_which == R_REGEN)
      player_restore_health(1, false);
  }

  if (ohp != player_get_health())
    quiet_rounds = 0;
}

/** daemon_start_wanderer
 * Called when it is time to start rolling for wandering monsters */
void
daemon_start_wanderer(void)
{
  daemon_start(daemon_rollwand, 0, BEFORE);
}

/** daemon_rollwand:
 * Called to roll to see if a wandering monster starts up */
void
daemon_rollwand(void)
{
  static int between = 4;

  if (++between >= 4)
  {
    if (roll(1, 6) == 4)
    {
      monster_new_random_wanderer();
      daemon_kill(daemon_rollwand);
      daemon_start_fuse(daemon_start_wanderer, 0, WANDERTIME, BEFORE);
    }
    between = 0;
  }
}

/** daemon_digest_food:
 * Digest the hero's food */
void
daemon_digest_food(void)
{
  int const hungry_time = 300;
  int const starving_time = 150;

  /* Player is dying from lack of food */
  if (food_left <= 0)
  {
    if (food_left-- < -STARVETIME)
      death('s');

    /** the hero is fainting */
    if (no_command || rnd(5) != 0)
      return;

    no_command += rnd(8) + 4;
    hungry_state = 3;
    msg("you faint from lack of food");
    command_stop(true);
    return;
  }


  int oldfood = food_left;
  food_left -= ring_drain_amount() + 1 - pack_contains_amulet();

  if (food_left < starving_time && oldfood >= starving_time)
  {
    hungry_state = 2;
    msg("you feel weak from lack of food");
    command_stop(true);
    return;
  }

  if (food_left < hungry_time && oldfood >= hungry_time)
  {
    hungry_state = 1;
    msg("you are starting to get hungry");
    command_stop(true);
    return;
  }
}


/** daemon_change_visuals:
 * change the characters for the player */
void
daemon_change_visuals(void)
{
  if (!after || (running && jump))
    return;

  /* change the things */
  for (THING* tp = lvl_obj; tp != NULL; tp = tp->l_next)
    if (cansee(tp->o_pos.y, tp->o_pos.x))
      mvaddcch(tp->o_pos.y, tp->o_pos.x, rnd_thing());

  /* change the stairs */
  if (seen_stairs())
    mvaddcch(stairs.y, stairs.x, rnd_thing());

  /* change the monsters */
  bool seemonst = player_can_sense_monsters();
  for (THING* tp = mlist; tp != NULL; tp = tp->l_next)
  {
    if (see_monst(tp))
    {
      if (tp->t_type == 'X' && tp->t_disguise != 'X')
        mvaddcch(tp->t_pos.y, tp->t_pos.x, rnd_thing());
      else
        mvaddcch(tp->t_pos.y, tp->t_pos.x, rnd(26) + 'A');
    }
    else if (seemonst)
      mvaddcch(tp->t_pos.y, tp->t_pos.x, (rnd(26) + 'A') | A_STANDOUT);
  }
}

/** daemon_runners_move
 * Make all running monsters move */
void
daemon_runners_move(void)
{
  THING* next;

  for (THING* tp = mlist; tp != NULL; tp = next)
  {
    /* remember this in case the monster's "next" is changed */
    next = tp->l_next;

    if (!monster_is_held(tp) && monster_is_chasing(tp))
    {
      bool wastarget = monster_is_players_target(tp);
      coord orig_pos = tp->t_pos;
      if (!monster_chase(tp))
        continue;

      assert_attached(mlist, tp);

      if (monster_is_flying(tp) && dist_cp(player_get_pos(), &tp->t_pos) >= 3)
        monster_chase(tp);

      assert_attached(mlist, tp);

      if (wastarget && !same_coords(&orig_pos, &tp->t_pos))
      {
        tp->t_flags &= ~ISTARGET;
        to_death = false;
      }
    }
  }
}

void daemon_ring_abilities(void)
{
  for (int i = 0; i < RING_SLOTS_SIZE; ++i)
  {
    THING* obj = pack_equipped_item(ring_slots[i]);
    if (obj == NULL)
      continue;

    else if (obj->o_which == R_SEARCH)
      player_search();
    else if (obj->o_which == R_TELEPORT && rnd(50) == 0)
      player_teleport(NULL);
  }
}
