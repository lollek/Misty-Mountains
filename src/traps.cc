#include <assert.h>

#include <string>

using namespace std;

#include "game.h"
#include "coordinate.h"
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
  Game::new_level(Game::current_level + 1);
  io_msg("you fell into a trap!");
  return T_DOOR;
}

static enum trap_t
trap_door_monster(Monster* victim)
{
  if (monster_seen_by_player(victim))
  {
    io_msg("%s fell through the floor", victim->get_name().c_str());
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
trap_bear_monster(Monster* victim)
{
  if (monster_seen_by_player(victim))
  {
    io_msg("%s was caught in a bear trap", victim->get_name().c_str());
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
    case 1: io_msg("the light in here suddenly seems %s", color_random().c_str()); break;
    case 2: io_msg("you feel a sting in the side of your neck"); break;
    case 3: io_msg("multi-colored lines swirl around you, then fade"); break;
    case 4: io_msg("a %s light flashes in your eyes", color_random().c_str()); break;
    case 5: io_msg("a spike shoots past your ear!"); break;
    case 6: io_msg("%s sparks dance across your armor", color_random().c_str()); break;
    case 7: io_msg("you suddenly feel very thirsty"); break;
    case 8: io_msg("you feel time speed up suddenly"); break;
    case 9: io_msg("time now seems to be going slower"); break;
    case 10: io_msg("you pack turns %s!", color_random().c_str()); break;
  }
  return T_MYST;
}

static enum trap_t
trap_myst_monster(Monster* victim)
{
  if (monster_seen_by_player(victim))
  {
    io_msg("%s seems to have stepped on something", victim->get_name().c_str());
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
trap_sleep_monster(Monster* victim)
{
  if (monster_seen_by_player(victim))
  {
    io_msg("%s collapsed to the ground", victim->get_name().c_str());
  }
  monster_become_held(victim);
  return T_SLEEP;
}

static enum trap_t
trap_arrow_player(void)
{
  if (fight_swing_hits(player_get_level() - 1, player->get_armor(), 1))
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
    Item* arrow = weapon_create(ARROW, false);
    arrow->o_count = 1;
    arrow->set_pos(*player_get_pos());
    weapon_missile_fall(arrow, false);
    io_msg("an arrow shoots past you");
  }
  return T_ARROW;
}

static enum trap_t
trap_arrow_monster(Monster* victim)
{
  if (fight_swing_hits(victim->t_stats.s_lvl -1,
        victim->get_armor(), 1))
  {
    victim->t_stats.s_hpt -= roll(1,6);
    if (victim->t_stats.s_hpt <= 0)
    {
      monster_on_death(victim, false);
      if (monster_seen_by_player(victim))
        io_msg("An arrow killed %s", victim->get_name().c_str());
    }
    else if (monster_seen_by_player(victim))
      io_msg("An arrow shot %s", victim->get_name().c_str());
  }
  else
  {
    Item* arrow = weapon_create(ARROW, false);
    arrow->o_count = 1;
    arrow->set_pos(victim->t_pos);
    weapon_missile_fall(arrow, false);
    if (monster_seen_by_player(victim))
      io_msg("An arrow barely missed %s", victim->get_name().c_str());
  }
  return T_ARROW;
}

static enum trap_t
trap_telep_player(Coordinate* trap_coord)
{
  player_teleport(nullptr);
  mvaddcch(trap_coord->y, trap_coord->x, TRAP); /* Mark trap before we leave */
  return T_TELEP;
}

static enum trap_t
trap_telep_monster(Monster* victim)
{
  bool was_seen = monster_seen_by_player(victim);
  if (was_seen)
  {
    io_msg_add("%s ", victim->get_name().c_str());
  }

  monster_teleport(victim, nullptr);
  if (was_seen)
  {
    if (monster_seen_by_player(victim))
      io_msg("teleported a short distance");
    else
      io_msg("disappeared");
  }

  if (!was_seen && monster_seen_by_player(victim))
  {
    io_msg("%s appeared out of thin air", victim->get_name().c_str());
  }

  return T_TELEP;
}

static enum trap_t
trap_dart_player(void)
{
  if (!fight_swing_hits(player_get_level() + 1, player->get_armor(), 1))
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
trap_dart_monster(Monster* victim)
{
  /* TODO: In the future this should probably weaken the monster */
  if (fight_swing_hits(victim->t_stats.s_lvl + 1,
        victim->get_armor(), 1))
  {
    victim->t_stats.s_hpt -= roll(1,4);
    if (victim->t_stats.s_hpt <= 0)
    {
      monster_on_death(victim, false);
      if (monster_seen_by_player(victim))
      {
        io_msg("A poisoned dart killed %s", victim->get_name().c_str());
      }
    }
    else if (monster_seen_by_player(victim))
    {
      io_msg("An dart hit %s", victim->get_name().c_str());
    }
  }
  else if (monster_seen_by_player(victim))
  {
    io_msg("A dart barely missed %s", victim->get_name().c_str());
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
trap_rust_monster(Monster* victim)
{
  if (monster_seen_by_player(victim))
  {
    io_msg("a gush of water hits %s", victim->get_name().c_str());
  }
  return T_RUST;
}

enum trap_t
trap_spring(Monster* victim, Coordinate* trap_coord)
{
  assert(trap_coord != nullptr);

  bool is_player = victim == nullptr;
  char tr = Game::level->get_trap_type(*trap_coord);

  if (is_player) {
    /* If we're levitating, we won't trigger the trap */
    if (player_is_levitating()) {
      return T_RUST; /* this needs to be neither T_DOOR nor T_TELEP */
    }
    command_stop(true);
  }

  if (victim == nullptr || monster_seen_by_player(victim)) {
    Game::level->set_ch(*trap_coord, TRAP);
    Game::level->set_flag_seen(*trap_coord);
  }

  switch (tr)
  {
    case T_DOOR:
      if (is_player) return trap_door_player();
      else        return trap_door_monster(victim);
    case T_BEAR:
      if (is_player) return trap_bear_player();
      else        return trap_bear_monster(victim);
    case T_MYST:
      if (is_player) return trap_myst_player();
      else        return trap_myst_monster(victim);
    case T_SLEEP:
      if (is_player) return trap_sleep_player();
      else        return trap_sleep_monster(victim);
    case T_ARROW:
      if (is_player) return trap_arrow_player();
      else        return trap_arrow_monster(victim);
    case T_TELEP:
      if (is_player) return trap_telep_player(trap_coord);
      else        return trap_telep_monster(victim);
    case T_DART:
      if (is_player) return trap_dart_player();
      else        return trap_dart_monster(victim);
    case T_RUST:
      if (is_player) return trap_rust_player();
      else        return trap_rust_monster(victim);
    default:
      assert(0);
      return T_MYST;
  }
}


