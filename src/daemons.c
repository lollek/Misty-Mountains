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
#include "rogue.h"

#include "daemons.h"

#define EMPTY 0
#define DAEMON -1
#define MAXDAEMONS 20

static int quiet_rounds = 0;

static struct delayed_action d_list[MAXDAEMONS] = {
    { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY },
    { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY },
    { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY },
    { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY }, { EMPTY },
};

void *__daemons_ptr(void) { return d_list; }

/** daemon_empty_slot:
 * Find an empty slot in the daemon/fuse list */
static struct delayed_action *
daemon_empty_slot(void)
{
  struct delayed_action *dev;

  for (dev = d_list; dev <= &d_list[MAXDAEMONS-1]; dev++)
    if (dev->d_type == EMPTY)
      return dev;

  msg("DEBUG: Ran out of fuse slots :(");
  return NULL;
}

/** daemon_find_slot:
 * Find a particular slot in the table */
static struct delayed_action *
daemon_find_slot(void (*func)())
{
  struct delayed_action *dev;

  for (dev = d_list; dev <= &d_list[MAXDAEMONS-1]; dev++)
    if (dev->d_type != EMPTY && func == dev->d_func)
      return dev;

  return NULL;
}

/** daemon_start:
 * Start a daemon, takes a function. */
void
daemon_start(void (*func)(), int arg, int type)
{
  struct delayed_action *dev = daemon_empty_slot();
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
  struct delayed_action *dev = daemon_find_slot(func);
  if (dev != NULL)
    dev->d_type = EMPTY;
}

/** daemon_run_all:
 * Run all the daemons that are active with the current flag,
 * passing the argument to the function. */
void
daemon_run_all(int flag)
{
  struct delayed_action *dev;

  for (dev = d_list; dev <= &d_list[MAXDAEMONS-1]; dev++)
    if (dev->d_type == flag && dev->d_time == DAEMON)
      (*dev->d_func)(dev->d_arg);
}

/** fuse:
 * Start a fuse to go off in a certain number of turns */
void
daemon_start_fuse(void (*func)(), int arg, int time, int type)
{
  struct delayed_action *wire = daemon_empty_slot();
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
  struct delayed_action *wire = daemon_find_slot(func);
  if (wire != NULL)
    wire->d_time += xtime;
}

/** daemon_extinguish_fuse:
 * Put out a fuse */
void
daemon_extinguish_fuse(void (*func)())
{
  struct delayed_action *wire = daemon_find_slot(func);
  if (wire != NULL)
    wire->d_type = EMPTY;
}

/** daemon_run_fuses:
 * Decrement counters and start needed fuses */
void
daemon_run_fuses(int flag)
{
  struct delayed_action *wire;

  for (wire = d_list; wire <= &d_list[MAXDAEMONS-1]; wire++)
    if (flag == wire->d_type && wire->d_time > 0 && --wire->d_time == 0)
    {
      wire->d_type = EMPTY;
      (*wire->d_func)(wire->d_arg);
    }
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
  int lv = player_get_level();
  int ohp = player_get_health();
  int i;

  if (ohp == player_get_max_health())
    return;

  quiet_rounds++;
  if (lv < 8)
  {
    if (quiet_rounds + (lv << 1) > 20)
      player_restore_health(1, false);
  }
  else if (quiet_rounds >= 3)
    player_restore_health(rnd(lv - 7) + 1, false);

  for (i = 0; i < RING_SLOTS_SIZE; ++i)
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
  int oldfood;
  int orig_hungry = hungry_state;

  if (food_left <= 0)
  {
    if (food_left-- < -STARVETIME)
      death('s');

    /** the hero is fainting */
    if (no_command || rnd(5) != 0)
      return;

    no_command += rnd(8) + 4;
    hungry_state = 3;
    if (!terse)
      addmsg(player_is_hallucinating()
          ? "the munchies overpower your motor capabilities.  "
          : "you feel too weak from lack of food.  ");
    msg(player_is_hallucinating()
        ? "You freak out"
        : "You faint");
  }
  else
  {
    oldfood = food_left;
    food_left -= ring_drain_amount() + 1 - pack_contains_amulet();

    if (food_left < MORETIME && oldfood >= MORETIME)
    {
      hungry_state = 2;
      msg(player_is_hallucinating()
          ? "the munchies are interfering with your motor capabilites"
          : "you are starting to feel weak");
    }
    else if (food_left < 2 * MORETIME && oldfood >= 2 * MORETIME)
    {
      hungry_state = 1;
      if (terse)
        msg(player_is_hallucinating()
            ? "getting the munchies"
            : "getting hungry");
      else
        msg(player_is_hallucinating()
            ? "you are getting the munchies"
            : "you are starting to get hungry");
    }
  }
  if (hungry_state != orig_hungry)
    command_stop(true);
}


/** daemon_change_visuals:
 * change the characters for the player */
void
daemon_change_visuals(void)
{
  THING *tp;
  bool seemonst;

  if (!after || (running && jump))
    return;

  /* change the things */
  for (tp = lvl_obj; tp != NULL; tp = tp->l_next)
    if (cansee(tp->o_pos.y, tp->o_pos.x))
      mvaddcch(tp->o_pos.y, tp->o_pos.x, rnd_thing());

  /* change the stairs */
  if (!seen_stairs())
    mvaddcch(stairs.y, stairs.x, rnd_thing());

  /* change the monsters */
  seemonst = player_can_sense_monsters();
  for (tp = mlist; tp != NULL; tp = tp->l_next)
  {
    move(tp->t_pos.y, tp->t_pos.x);
    if (see_monst(tp))
    {
      if (tp->t_type == 'X' && tp->t_disguise != 'X')
        addcch(rnd_thing());
      else
        addcch(rnd(26) + 'A');
    }
    else if (seemonst)
      addcch((rnd(26) + 'A') | A_STANDOUT);
  }
}

/** daemon_runners_move
 * Make all running monsters move */
void
daemon_runners_move(void)
{
  THING *tp;
  THING *next;

  for (tp = mlist; tp != NULL; tp = next)
  {
    /* remember this in case the monster's "next" is changed */
    next = tp->l_next;

    if (!on(*tp, ISHELD) && on(*tp, ISRUN))
    {
      bool wastarget = on(*tp, ISTARGET);
      coord orig_pos = tp->t_pos;
      if (!monster_chase(tp))
        continue;

      assert(tp != NULL);

      if (on(*tp, ISFLY) && dist_cp(player_get_pos(), &tp->t_pos) >= 3)
        monster_chase(tp);

      assert(tp != NULL);

      if (wastarget && !same_coords(orig_pos, tp->t_pos))
      {
        tp->t_flags &= ~ISTARGET;
        to_death = false;
      }
    }
  }

  if (has_hit)
  {
    endmsg();
    has_hit = false;
  }
}
