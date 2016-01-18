#include <assert.h>

#include "game.h"
#include "io.h"
#include "pack.h"
#include "command.h"
#include "monster.h"
#include "rings.h"
#include "misc.h"
#include "player.h"
#include "level.h"
#include "options.h"
#include "os.h"
#include "rogue.h"

#include "daemons.h"

#define EMPTY 0
#define DAEMON -1
#define MAXDAEMONS 20

static int quiet_rounds = 0;

static struct delayed_action daemons[MAXDAEMONS];

void* __daemons_ptr() { return daemons; }

/** daemon_empty_slot:
 * Find an empty slot in the daemon/fuse list */
static struct delayed_action*
daemon_empty_slot()
{
  for (int i = 0; i < MAXDAEMONS; ++i)
    if (daemons[i].d_type == EMPTY)
      return &daemons[i];

  io_msg("DEBUG: Ran out of fuse slots :(");
  return nullptr;
}

/** daemon_find_slot:
 * Find a particular slot in the table */
static struct delayed_action*
daemon_find_slot(void (*func)(int))
{
  for (int i = 0; i < MAXDAEMONS; ++i)
    if (daemons[i].d_type != EMPTY && daemons[i].d_func == func)
      return &daemons[i];

  return nullptr;
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

void daemon_run_before()
{
  daemon_run_all(BEFORE);
  daemon_run_fuses(BEFORE);
}

void daemon_run_after()
{
  daemon_run_all(AFTER);
  daemon_run_fuses(AFTER);
}


/** daemon_start:
 * Start a daemon, takes a function. */
void
daemon_start(void (*func)(int), int arg, int type)
{
  struct delayed_action* dev = daemon_empty_slot();
  if (dev != nullptr)
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
daemon_kill(void (*func)(int))
{
  struct delayed_action* dev = daemon_find_slot(func);
  if (dev != nullptr)
    dev->d_type = EMPTY;
}


/** fuse:
 * Start a fuse to go off in a certain number of turns */
void
daemon_start_fuse(void (*func)(int), int arg, int time, int type)
{
  struct delayed_action* wire = daemon_empty_slot();
  if (wire != nullptr)
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
daemon_lengthen_fuse(void (*func)(int), int xtime)
{
  struct delayed_action* wire = daemon_find_slot(func);
  if (wire != nullptr)
    wire->d_time += xtime;
}

/** daemon_extinguish_fuse:
 * Put out a fuse */
void
daemon_extinguish_fuse(void (*func)(int))
{
  struct delayed_action* wire = daemon_find_slot(func);
  if (wire != nullptr)
    wire->d_type = EMPTY;
}


/** daemon_reset_doctor
 * Stop the daemon doctor from healing */
void
daemon_reset_doctor(__attribute__((unused)) int)
{
  quiet_rounds = 0;
}

/** daemon_doctor:
 * A healing daemon that restors hit points after rest */
void
daemon_doctor(__attribute__((unused)) int)
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
    player_restore_health(os_rand_range(player_get_level() - 7) + 1, false);

  for (int i = 0; i < PACK_RING_SLOTS; ++i)
  {
    Item *ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring != nullptr && ring->o_which == R_REGEN)
      player_restore_health(1, false);
  }

  if (ohp != player_get_health())
    quiet_rounds = 0;
}

/** daemon_start_wanderer
 * Called when it is time to start rolling for wandering monsters */
void
daemon_start_wanderer(__attribute__((unused)) int)
{
  daemon_start(daemon_rollwand, 0, BEFORE);
}

/** daemon_rollwand:
 * Called to roll to see if a wandering monster starts up */
void
daemon_rollwand(__attribute__((unused)) int)
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

/** daemon_change_visuals:
 * change the characters for the player */
void
daemon_change_visuals(__attribute__((unused)) int)
{
  if (running && jump)
    return;

  /* change the things */
  for (Item* tp : Game::level->items)
    if (player->can_see(tp->get_pos()))
      mvaddcch(tp->get_y(), tp->get_x(), static_cast<chtype>(rnd_thing()));

  /* change the stairs */
  if (player->has_seen_stairs())
    mvaddcch(Game::level->get_stairs_y(),
             Game::level->get_stairs_x(),
             static_cast<chtype>(rnd_thing()));

  /* change the monsters */
  monster_show_all_as_trippy();
}

/** daemon_runners_move
 * Make all running monsters move */
void
daemon_runners_move(__attribute__((unused)) int)
{
  monster_move_all();
}

void daemon_ring_abilities(__attribute__((unused)) int)
{
  for (int i = 0; i < PACK_RING_SLOTS; ++i)
  {
    Item* obj = pack_equipped_item(pack_ring_slots[i]);
    if (obj == nullptr)
      continue;

    else if (obj->o_which == R_SEARCH)
      player_search();
    else if (obj->o_which == R_TELEPORT && os_rand_range(50) == 0)
      player_teleport(nullptr);
  }
}
