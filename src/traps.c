
#include <assert.h>

#include "command.h"
#include "io.h"
#include "armor.h"
#include "fight.h"
#include "colors.h"
#include "list.h"
#include "level.h"
#include "rings.h"
#include "misc.h"
#include "weapons.h"
#include "monster.h"
#include "rip.h"
#include "list.h"
#include "player.h"

#include "traps.h"


const char *trap_names[NTRAPS] = {
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
  msg("you fell into a trap!");
  return T_DOOR;
}

static enum trap_t
trap_door_monster(THING *victim)
{
  assert_monster(victim);

  if (see_monst(victim))
    msg("%s fell through the floor", set_mname(victim));
  monster_remove_from_screen(&victim->t_pos, victim, false);
  return T_DOOR;
}

static enum trap_t
trap_bear_player(void)
{
  player_become_stuck();
  msg("you are caught in a bear trap");
  return T_BEAR;
}

static enum trap_t
trap_bear_monster(THING *victim)
{
  assert_monster(victim);

  if (see_monst(victim))
    msg("%s was caught in a bear trap", set_mname(victim));
  monster_become_stuck(victim);
  return T_BEAR;
}

static enum trap_t
trap_myst_player(void)
{
  switch(rnd(11))
  {
    case 0: msg("you are suddenly in a parallel dimension"); break;
    case 1: msg("the light in here suddenly seems %s", colors_random()); break;
    case 2: msg("you feel a sting in the side of your neck"); break;
    case 3: msg("multi-colored lines swirl around you, then fade"); break;
    case 4: msg("a %s light flashes in your eyes", colors_random()); break;
    case 5: msg("a spike shoots past your ear!"); break;
    case 6: msg("%s sparks dance across your armor", colors_random()); break;
    case 7: msg("you suddenly feel very thirsty"); break;
    case 8: msg("you feel time speed up suddenly"); break;
    case 9: msg("time now seems to be going slower"); break;
    case 10: msg("you pack turns %s!", colors_random()); break;
  }
  return T_MYST;
}

static enum trap_t
trap_myst_monster(THING *victim)
{
  assert_monster(victim);

  if (see_monst(victim))
    msg("%s seems to have stepped on something", set_mname(victim));
  return T_MYST;
}

static enum trap_t
trap_sleep_player(void)
{
  player_fall_asleep();
  addmsg("a strange white mist envelops you and ");
  return T_SLEEP;
}

static enum trap_t
trap_sleep_monster(THING *victim)
{
  assert_monster(victim);

  if (see_monst(victim))
    msg("%s collapsed to the ground", set_mname(victim));
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
      msg("an arrow killed you");
      death('a');
    }
    else
      msg("oh no! An arrow shot you");
  }
  else
  {
    THING *arrow = new_item();
    init_weapon(arrow, ARROW);
    arrow->o_count = 1;
    arrow->o_pos = *player_get_pos();
    fall(arrow, false);
    msg("an arrow shoots past you");
  }
  return T_ARROW;
}

static enum trap_t
trap_arrow_monster(THING *victim)
{
  assert_monster(victim);

  if (fight_swing_hits(victim->t_stats.s_lvl -1, armor_for_thing(victim), 1))
  {
    victim->t_stats.s_hpt -= roll(1,6);
    if (victim->t_stats.s_hpt <= 0)
    {
      monster_on_death(victim, false);
      if (see_monst(victim))
        msg("An arrow killed %s", set_mname(victim));
    }
    else if (see_monst(victim))
      msg("An arrow shot %s", set_mname(victim));
  }
  else
  {
    THING *arrow = new_item();
    init_weapon(arrow, ARROW);
    arrow->o_count = 1;
    arrow->o_pos = victim->t_pos;
    fall(arrow, false);
    if (see_monst(victim))
      msg("An arrow barely missed %s", set_mname(victim));
  }
  return T_ARROW;
}

static enum trap_t
trap_telep_player(coord *trap_coord)
{
  player_teleport(NULL);
  mvaddcch(trap_coord->y, trap_coord->x, TRAP); /* Mark trap before we leave */
  return T_TELEP;
}

static enum trap_t
trap_telep_monster(THING *victim)
{
  bool was_seen;

  assert_monster(victim);

  was_seen = see_monst(victim);
  if (was_seen)
    addmsg("%s ", set_mname(victim));

  monster_teleport(victim, NULL);
  if (was_seen)
  {
    if (see_monst(victim))
      msg("teleported a short distance");
    else
      msg("disappeared");
  }

  if (!was_seen && see_monst(victim))
    msg("%s appeared out of thin air", set_mname(victim));

  return T_TELEP;
}

static enum trap_t
trap_dart_player(void)
{
  if (!fight_swing_hits(player_get_level() + 1, player_get_armor(), 1))
    msg("a small dart whizzes by your ear and vanishes");
  else
  {
    player_lose_health(roll(1, 4));
    if (player_get_health() <= 0)
    {
      msg("a poisoned dart killed you");
      death('d');
    }
    if (!player_has_ring_with_ability(R_SUSTSTR) && !player_save_throw(VS_POISON))
      player_modify_strength(-1);
    msg("a small dart just hit you in the shoulder");
  }
  return T_DART;
}

static enum trap_t
trap_dart_monster(THING *victim)
{
  assert_monster(victim);

  /* TODO: In the future this should probably weaken the monster */
  if (fight_swing_hits(victim->t_stats.s_lvl + 1, armor_for_thing(victim), 1))
  {
    victim->t_stats.s_hpt -= roll(1,4);
    if (victim->t_stats.s_hpt <= 0)
    {
      monster_on_death(victim, false);
      if (see_monst(victim))
        msg("A poisoned dart killed %s", set_mname(victim));
    }
    else if (see_monst(victim))
      msg("An dart hit %s", set_mname(victim));
  }
  else if (see_monst(victim))
    msg("A dart barely missed %s", set_mname(victim));
  return T_DART;
}

static enum trap_t
trap_rust_player(void)
{
  msg("a gush of water hits you on the head");
  armor_rust();
  return T_RUST;
}

static enum trap_t
trap_rust_monster(THING *victim)
{
  assert_monster(victim);
  if (see_monst(victim))
    msg("a gush of water hits %s", set_mname(victim));
  return T_RUST;
}

enum trap_t
be_trapped(THING *victim, coord *trap_coord)
{
  bool player = victim == NULL;
  PLACE *pp;
  char tr;

  assert(trap_coord != NULL);
  pp = INDEX(trap_coord->y, trap_coord->x);
  tr = pp->p_flags & F_TMASK;

  if (player)
  {
    /* If we're levitating, we won't trigger the trap */
    if (player_is_levitating())
      return T_RUST; /* this needs to be neither T_DOOR nor T_TELEP */
    command_stop(true);
  }

  if (victim == NULL || see_monst(victim))
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


