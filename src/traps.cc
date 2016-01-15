#include <assert.h>

#include <string>

using namespace std;

#include "Coordinate.h"
#include "command.h"
#include "io.h"
#include "armor.h"
#include "fight.h"
#include "colors.h"
#include "level.h"
#include "rings.h"
#include "misc.h"
#include "weapons.h"
#include "monster.h"
#include "os.h"
#include "player.h"
#include "death.h"

#include "traps.h"

string const trap_names[] = {
  "a trapdoor",
  "an arrow trap",
  "a sleeping gas trap",
  "a beartrap",
  "a teleport trap",
  "a poison dart trap",
  "a rust trap",
  "a mysterious trap"
};

static enum trap_t
trap_door_player(void)
{
  level++;
  level_new();
  io_msg("you fell into a trap!");
  return T_DOOR;
}

static enum trap_t
trap_door_monster(monster* victim)
{
  if (monster_seen_by_player(victim))
  {
    char buf[MAXSTR];
    io_msg("%s fell through the floor", monster_name(victim, buf));
  }
  monster_remove_from_screen(&victim->t_pos, victim, false);
  return T_DOOR;
}

static enum trap_t
trap_bear_player(void)
{
  player_become_stuck();
  io_msg("you are caught in a bear trap");
  return T_BEAR;
}

static enum trap_t
trap_bear_monster(monster* victim)
{
  if (monster_seen_by_player(victim))
  {
    char buf[MAXSTR];
    io_msg("%s was caught in a bear trap", monster_name(victim, buf));
  }
  monster_become_stuck(victim);
  return T_BEAR;
}

static enum trap_t
trap_myst_player(void)
{
  switch(os_rand_range(11))
  {
    case 0: io_msg("you are suddenly in a parallel dimension"); break;
    case 1: io_msg("the light in here suddenly seems %s", color_random()); break;
    case 2: io_msg("you feel a sting in the side of your neck"); break;
    case 3: io_msg("multi-colored lines swirl around you, then fade"); break;
    case 4: io_msg("a %s light flashes in your eyes", color_random()); break;
    case 5: io_msg("a spike shoots past your ear!"); break;
    case 6: io_msg("%s sparks dance across your armor", color_random()); break;
    case 7: io_msg("you suddenly feel very thirsty"); break;
    case 8: io_msg("you feel time speed up suddenly"); break;
    case 9: io_msg("time now seems to be going slower"); break;
    case 10: io_msg("you pack turns %s!", color_random()); break;
  }
  return T_MYST;
}

static enum trap_t
trap_myst_monster(monster* victim)
{
  if (monster_seen_by_player(victim))
  {
    char buf[MAXSTR];
    io_msg("%s seems to have stepped on something", monster_name(victim, buf));
  }
  return T_MYST;
}

static enum trap_t
trap_sleep_player(void)
{
  player_fall_asleep();
  io_msg_add("a strange white mist envelops you and ");
  return T_SLEEP;
}

static enum trap_t
trap_sleep_monster(monster* victim)
{
  if (monster_seen_by_player(victim))
  {
    char buf[MAXSTR];
    io_msg("%s collapsed to the ground", monster_name(victim, buf));
  }
  monster_become_held(victim);
  return T_SLEEP;
}

static enum trap_t
trap_arrow_player(void)
{
  if (fight_swing_hits(player_get_level() - 1, player_get_armor(), 1))
  {
    player_lose_health(roll(1, 6));
    if (player_get_health() <= 0)
    {
      io_msg("an arrow killed you");
      death(DEATH_ARROW);
    }
    else
      io_msg("oh no! An arrow shot you");
  }
  else
  {
    item* arrow = weapon_create(ARROW, false);
    arrow->o_count = 1;
    arrow->o_pos = *player_get_pos();
    weapon_missile_fall(arrow, false);
    io_msg("an arrow shoots past you");
  }
  return T_ARROW;
}

static enum trap_t
trap_arrow_monster(monster* victim)
{
  char buf[MAXSTR];

  if (fight_swing_hits(victim->t_stats.s_lvl -1,
        armor_for_monster(victim), 1))
  {
    victim->t_stats.s_hpt -= roll(1,6);
    if (victim->t_stats.s_hpt <= 0)
    {
      monster_on_death(victim, false);
      if (monster_seen_by_player(victim))
        io_msg("An arrow killed %s", monster_name(victim, buf));
    }
    else if (monster_seen_by_player(victim))
      io_msg("An arrow shot %s", monster_name(victim, buf));
  }
  else
  {
    item* arrow = weapon_create(ARROW, false);
    arrow->o_count = 1;
    arrow->o_pos = victim->t_pos;
    weapon_missile_fall(arrow, false);
    if (monster_seen_by_player(victim))
      io_msg("An arrow barely missed %s", monster_name(victim, buf));
  }
  return T_ARROW;
}

static enum trap_t
trap_telep_player(Coordinate* trap_coord)
{
  player_teleport(NULL);
  mvaddcch(trap_coord->y, trap_coord->x, TRAP); /* Mark trap before we leave */
  return T_TELEP;
}

static enum trap_t
trap_telep_monster(monster* victim)
{
  bool was_seen = monster_seen_by_player(victim);
  if (was_seen)
  {
    char buf[MAXSTR];
    io_msg_add("%s ", monster_name(victim, buf));
  }

  monster_teleport(victim, NULL);
  if (was_seen)
  {
    if (monster_seen_by_player(victim))
      io_msg("teleported a short distance");
    else
      io_msg("disappeared");
  }

  if (!was_seen && monster_seen_by_player(victim))
  {
    char buf[MAXSTR];
    io_msg("%s appeared out of thin air", monster_name(victim, buf));
  }

  return T_TELEP;
}

static enum trap_t
trap_dart_player(void)
{
  if (!fight_swing_hits(player_get_level() + 1, player_get_armor(), 1))
    io_msg("a small dart whizzes by your ear and vanishes");
  else
  {
    player_lose_health(roll(1, 4));
    if (player_get_health() <= 0)
    {
      io_msg("a poisoned dart killed you");
      death(DEATH_DART);
    }
    if (!player_has_ring_with_ability(R_SUSTSTR) && !player_save_throw(VS_POISON))
      player_modify_strength(-1);
    io_msg("a small dart just hit you in the shoulder");
  }
  return T_DART;
}

static enum trap_t
trap_dart_monster(monster* victim)
{
  /* TODO: In the future this should probably weaken the monster */
  if (fight_swing_hits(victim->t_stats.s_lvl + 1,
        armor_for_monster(victim), 1))
  {
    victim->t_stats.s_hpt -= roll(1,4);
    if (victim->t_stats.s_hpt <= 0)
    {
      monster_on_death(victim, false);
      if (monster_seen_by_player(victim))
      {
        char buf[MAXSTR];
        io_msg("A poisoned dart killed %s", monster_name(victim, buf));
      }
    }
    else if (monster_seen_by_player(victim))
    {
      char buf[MAXSTR];
      io_msg("An dart hit %s", monster_name(victim, buf));
    }
  }
  else if (monster_seen_by_player(victim))
  {
    char buf[MAXSTR];
    io_msg("A dart barely missed %s", monster_name(victim, buf));
  }
  return T_DART;
}

static enum trap_t
trap_rust_player(void)
{
  io_msg("a gush of water hits you on the head");
  armor_rust();
  return T_RUST;
}

static enum trap_t
trap_rust_monster(monster* victim)
{
  if (monster_seen_by_player(victim))
  {
    char buf[MAXSTR];
    io_msg("a gush of water hits %s", monster_name(victim, buf));
  }
  return T_RUST;
}

enum trap_t
trap_spring(monster* victim, Coordinate* trap_coord)
{
  assert(trap_coord != NULL);

  bool player = victim == NULL;
  PLACE* pp = level_get_place(trap_coord->y, trap_coord->x);
  char tr = pp->p_flags & F_TMASK;

  if (player)
  {
    /* If we're levitating, we won't trigger the trap */
    if (player_is_levitating())
      return T_RUST; /* this needs to be neither T_DOOR nor T_TELEP */
    command_stop(true);
  }

  if (victim == NULL || monster_seen_by_player(victim))
  {
    pp->p_ch = TRAP;
    pp->p_flags |= F_SEEN;
  }

  switch (tr)
  {
    case T_DOOR:
      if (player) return trap_door_player();
      else        return trap_door_monster(victim);
    case T_BEAR:
      if (player) return trap_bear_player();
      else        return trap_bear_monster(victim);
    case T_MYST:
      if (player) return trap_myst_player();
      else        return trap_myst_monster(victim);
    case T_SLEEP:
      if (player) return trap_sleep_player();
      else        return trap_sleep_monster(victim);
    case T_ARROW:
      if (player) return trap_arrow_player();
      else        return trap_arrow_monster(victim);
    case T_TELEP:
      if (player) return trap_telep_player(trap_coord);
      else        return trap_telep_monster(victim);
    case T_DART:
      if (player) return trap_dart_player();
      else        return trap_dart_monster(victim);
    case T_RUST:
      if (player) return trap_rust_player();
      else        return trap_rust_monster(victim);
    default:
      assert(0);
      return T_MYST;
  }
}


