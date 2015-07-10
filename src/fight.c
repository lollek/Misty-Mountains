/*
 * All the fighting gets done here
 *
 * @(#)fight.c	4.67 (Berkeley) 09/06/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "command.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "daemons.h"
#include "list.h"
#include "monster.h"
#include "rings.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "weapons.h"
#include "things.h"
#include "death.h"
#include "options.h"
#include "rogue.h"
#include "os.h"

#include "fight.h"

struct attack_modifier
{
  int to_hit;
  int to_dmg;
  struct damage damage[MAXATTACKS];
};

/** add_player_attack_modifiers
 * Add item bonuses to damage and to-hit */
static void
add_ring_attack_modifiers(struct attack_modifier* mod)
{
  for (int i = 0; i < PACK_RING_SLOTS; ++i)
  {
    THING* ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring == NULL)
      continue;

    else if (ring->o.o_which == R_ADDDAM)
      mod->to_dmg += ring->o.o_arm;
    else if (ring->o.o_which == R_ADDHIT)
      mod->to_dmg += ring->o.o_arm;
  }
}

static void
add_strength_attack_modifiers(int strength, struct attack_modifier* mod)
{
  switch (strength)
  {
    case  0: mod->to_hit -= 7; mod->to_dmg -= 7; return;
    case  1: mod->to_hit -= 6; mod->to_dmg -= 6; return;
    case  2: mod->to_hit -= 5; mod->to_dmg -= 5; return;
    case  3: mod->to_hit -= 4; mod->to_dmg -= 4; return;
    case  4: mod->to_hit -= 3; mod->to_dmg -= 3; return;
    case  5: mod->to_hit -= 2; mod->to_dmg -= 2; return;
    case  6: mod->to_hit -= 1; mod->to_dmg -= 1; return;

    default: return;

    case 16:                   mod->to_dmg += 1; return;
    case 17: mod->to_hit += 1; mod->to_dmg += 1; return;
    case 18: mod->to_hit += 1; mod->to_dmg += 2; return;
    case 19: mod->to_hit += 1; mod->to_dmg += 3; return;
    case 20: mod->to_hit += 1; mod->to_dmg += 3; return;
    case 21: mod->to_hit += 2; mod->to_dmg += 4; return;
    case 22: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 23: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 24: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 25: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 26: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 27: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 28: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 29: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 30: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 31: mod->to_hit += 3; mod->to_dmg += 6; return;
  }
}

static void
calculate_attacker(THING* attacker, THING* weapon, bool thrown,
                  struct attack_modifier* mod)
{
  if (weapon != NULL)
  {
    mod->damage[0] = weapon->o.o_damage;
    mod->to_hit += weapon->o.o_hplus;
    mod->to_dmg += weapon->o.o_dplus;
  }

  add_strength_attack_modifiers(attacker->t.t_stats.s_str, mod);

  /* Player stuff */
  if (is_player(&attacker->t))
  {
    add_ring_attack_modifiers(mod);
    if (thrown)
    {
      THING const* held_weapon = pack_equipped_item(EQUIPMENT_RHAND);
      if ((weapon->o.o_flags & ISMISL) && held_weapon != NULL
          && held_weapon->o.o_which == weapon->o.o_launch)
      {
        mod->damage[0] = weapon->o.o_hurldmg;
        mod->to_hit += held_weapon->o.o_hplus;
        mod->to_dmg += held_weapon->o.o_dplus;
      }
      else if (weapon->o.o_launch == -1)
        mod->damage[0] = weapon->o.o_hurldmg;
    }
  }

  /* Venus Flytraps have a different kind of dmg system */
  else if (attacker->o.o_type == 'F')
    mod->damage[0].sides = monster_flytrap_hit;
}

/* Roll attackers attack vs defenders defense and then take damage if it hits
 *
 * Attacker or defender can be NULL, in that case it's the player */
static bool
roll_attacks(THING* attacker, THING* defender, THING* weapon, bool thrown)
{
  /* TODO: remove __player_ptr reference */
       if (attacker == NULL) attacker = __player_ptr();
  else if (defender == NULL) defender = __player_ptr();

  struct attack_modifier mod;
  mod.to_hit = 0;
  mod.to_dmg = 0;
  assert(sizeof(mod.damage) == sizeof(attacker->t.t_stats.s_dmg));
  memcpy(mod.damage, attacker->t.t_stats.s_dmg, sizeof(attacker->t.t_stats.s_dmg));

  calculate_attacker(attacker, weapon, thrown, &mod);

  /* If defender is stuck in some way,the attacker gets a bonus to hit */
  if ((is_player(&defender->t) && player_turns_without_action)
      || monster_is_held(defender)
      || monster_is_stuck(defender))
    mod.to_hit += 4;

  assert(mod.damage[0].sides != -1 && mod.damage[0].dices != -1);

  bool did_hit = false;
  for (int i = 0; i < MAXATTACKS; ++i)
  {
    int dice_sides = mod.damage[i].sides;
    int dices = mod.damage[i].dices;

    if (dice_sides == 0 && dices == 0)
      continue;

    int defense = armor_for_monster(&defender->t);
    if (fight_swing_hits(attacker->t.t_stats.s_lvl, defense, mod.to_hit))
    {
      int damage = roll(dices, dice_sides) + mod.to_dmg;
      if (damage > 0)
        defender->t.t_stats.s_hpt -= damage;
      did_hit = true;
    }
  }

  return did_hit;
}

/** print_attack:
 * Print a message to indicate a hit or miss */
static void
print_attack(bool hit, char const* att, char const* def)
{
  char const* h_names[] = {
      "hit"
    , "scored an excellent hit on"
    , "have injured"
    , "swing and hit"
    , "hits"
    , "scored an excellent hit on"
    , "has injured"
    , "swings and hits"
  };
  char const* m_names[] = {
      "miss"
    , "swing and miss"
    , "barely miss"
    , "don't hit"
    , "misses"
    , "swings and misses"
    , "barely misses"
    , "doesn't hit"
  };

  int i = os_rand_range(4);
  if (att != NULL)
    i += 4;

  msg("%s %s %s",
      att == NULL ? "you" : att,
      hit ? h_names[i] : m_names[i],
      def == NULL ? "you" : def);
}

int
fight_against_monster(coord const* monster_pos, THING* weapon, bool thrown)
{
  THING* const player = NULL;
  THING* tp = level_get_monster(monster_pos->y, monster_pos->x);
  if (tp == NULL)
    return !fail("fight_against_monster(%p, %p, %b) NULL monster\r\n",
                 monster_pos, weapon, thrown);

  /* Since we are fighting, things are not quiet so no healing takes place */
  command_stop(false);
  daemon_reset_doctor();

  /* Let him know it was really a xeroc (if it was one) */
  if (tp->t.t_type == 'X' && tp->t.t_disguise != 'X' && !player_is_blind())
  {
      tp->t.t_disguise = 'X';
      msg("wait!  That's a xeroc!");
      if (!thrown)
          return false;
  }

  char mname[MAXSTR];
  monster_name(tp, mname);

  if (roll_attacks(player, tp, weapon, thrown))
  {
    if (tp->t.t_stats.s_hpt <= 0)
    {
      monster_on_death(tp, true);
      return true;
    }

    if (!to_death)
    {
      if (thrown)
      {
        if (weapon->o.o_type == WEAPON)
          addmsg("the %s hits ", weapon_info[weapon->o.o_which].oi_name);
        else
          addmsg("you hit ");
        msg("%s", mname);
      }
      else
        print_attack(true, (char *) NULL, mname);
    }

    if (player_has_confusing_attack())
    {
      monster_set_confused(tp);
      player_remove_confusing_attack();
      if (!player_is_blind())
      {
        msg("your hands stop glowing %s", pick_color("red"));
        msg("%s appears confused", mname);
      }
    }

    monster_start_running(monster_pos);
    return true;
  }
  monster_start_running(monster_pos);

  if (thrown && !to_death)
    fight_missile_miss(weapon, mname);
  else if (!to_death)
    print_attack(false, (char *) NULL, mname);
  return false;
}

int
fight_against_player(THING* mp)
{
  /* Since this is an attack, stop running and any healing that was
   * going on at the time */
  player_alerted = true;
  command_stop(false);
  daemon_reset_doctor();

  /* If we're fighting something to death and get backstabbed, return command */
  if (to_death && !monster_is_players_target(mp))
    to_death = false;

  /* If it's a xeroc, tag it as known */
  if (mp->t.t_type == 'X' && mp->t.t_disguise != 'X' && !player_is_blind()
      && !player_is_hallucinating())
    mp->t.t_disguise = 'X';

  char mname[MAXSTR];
  monster_name(mp, mname);

  if (roll_attacks(mp, NULL, NULL, false))
  {
    if (mp->t.t_type != 'I' && !to_death)
      print_attack(true, mname, (char *) NULL);

    if (player_get_health() <= 0)
      death(mp->t.t_type);

    monster_do_special_ability(&mp);
    /* N.B! mp can be null after this point! */

  }

  else if (mp->t.t_type != 'I')
  {
    if (mp->t.t_type == 'F')
    {
      player_lose_health(monster_flytrap_hit);
      if (player_get_health() <= 0)
        death(mp->t.t_type);
    }

    if (!to_death)
      print_attack(false, mname, (char *) NULL);
  }

  command_stop(false);
  status();
  return(mp == NULL ? -1 : 0);
}

int
fight_swing_hits(int at_lvl, int op_arm, int wplus)
{
  int rand = os_rand_range(20);
  /* msg("%d + %d + %d vs %d", at_lvl, wplus, rand, op_arm); */
  return at_lvl + wplus + rand >= op_arm;
}

void
fight_missile_miss(THING const* weap, char const* mname)
{
  if (weap->o.o_type == WEAPON)
    msg("the %s misses %s", weapon_info[weap->o.o_which].oi_name, mname);
  else
    msg("you missed %s", mname);
}
