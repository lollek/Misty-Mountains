#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <string>

using namespace std;

#include "game.h"
#include "colors.h"
#include "command.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "daemons.h"
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
    Item const* ring_thing = pack_equipped_item(pack_ring_slots[i]);
    if (ring_thing == nullptr)
      continue;

    Item const* ring = ring_thing;

    if (ring->o_which == R_ADDDAM)
      mod->to_dmg += ring->o_arm;
    else if (ring->o_which == R_ADDHIT)
      mod->to_dmg += ring->o_arm;
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
calculate_attacker(Monster* attacker, Item* weapon, bool thrown,
                  struct attack_modifier* mod)
{
  if (weapon != nullptr)
  {
    mod->damage[0] = weapon->o_damage;
    mod->to_hit += weapon->o_hplus;
    mod->to_dmg += weapon->o_dplus;
  }

  add_strength_attack_modifiers(attacker->t_stats.s_str, mod);

  /* Player stuff */
  if (attacker == player)
  {
    add_ring_attack_modifiers(mod);
    if (thrown)
    {
      Item const* held_weapon = pack_equipped_item(EQUIPMENT_RHAND);
      if ((weapon->o_flags & ISMISL) && held_weapon != nullptr
          && held_weapon->o_which == weapon->o_launch)
      {
        mod->damage[0] = weapon->o_hurldmg;
        mod->to_hit += held_weapon->o_hplus;
        mod->to_dmg += held_weapon->o_dplus;
      }
      else if (weapon->o_launch == -1)
        mod->damage[0] = weapon->o_hurldmg;
    }
  }

  /* Venus Flytraps have a different kind of dmg system */
  else if (attacker->t_type == 'F')
    mod->damage[0].sides = monster_flytrap_hit;
}

/* Roll attackers attack vs defenders defense and then take damage if it hits
 *
 * Attacker or defender can be nullptr, in that case it's the player */
static bool
roll_attacks(Monster* attacker, Monster* defender, Item* weapon, bool thrown)
{
       if (attacker == nullptr) attacker = player;
  else if (defender == nullptr) defender = player;

  struct attack_modifier mod;
  mod.to_hit = 0;
  mod.to_dmg = 0;
  assert(sizeof(mod.damage) == sizeof(attacker->t_stats.s_dmg));
  memcpy(mod.damage, attacker->t_stats.s_dmg, sizeof(attacker->t_stats.s_dmg));

  calculate_attacker(attacker, weapon, thrown, &mod);

  /* If defender is stuck in some way,the attacker gets a bonus to hit */
  if ((defender == player && player_turns_without_action)
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

    int defense = defender->get_armor();
    if (fight_swing_hits(attacker->t_stats.s_lvl, defense, mod.to_hit))
    {
      int damage = roll(dices, dice_sides) + mod.to_dmg;
      if (damage > 0)
        defender->t_stats.s_hpt -= damage;
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
  if (att != nullptr)
    i += 4;

  io_msg("%s %s %s",
      att == nullptr ? "you" : att,
      hit ? h_names[i] : m_names[i],
      def == nullptr ? "you" : def);
}

int
fight_against_monster(Coordinate const* monster_pos, Item* weapon, bool thrown)
{
  if (monster_pos == nullptr) {
    throw runtime_error("monster_pos was null");
  }
  Monster* tp = Game::level->get_monster(*monster_pos);
  if (tp == nullptr)
    return !io_fail("fight_against_monster(%p, %p, %b) nullptr monster\r\n",
                 monster_pos, weapon, thrown);

  /* Since we are fighting, things are not quiet so no healing takes place */
  command_stop(false);
  daemon_reset_doctor(0);

  /* Let him know it was really a xeroc (if it was one) */
  if (tp->t_type == 'X' && tp->t_disguise != 'X' && !player_is_blind())
  {
      tp->t_disguise = 'X';
      io_msg("wait!  That's a xeroc!");
      if (!thrown)
          return false;
  }

  char mname[MAXSTR];
  monster_name(tp, mname);

  if (roll_attacks(nullptr, tp, weapon, thrown))
  {
    if (tp->t_stats.s_hpt <= 0)
    {
      monster_on_death(tp, true);
      return true;
    }

    if (!to_death)
    {
      if (thrown)
      {
        if (weapon->o_type == WEAPON)
          io_msg_add("the %s hits ", weapon_info[static_cast<size_t>(weapon->o_which)].oi_name.c_str());
        else
          io_msg_add("you hit ");
        io_msg("%s", mname);
      }
      else
        print_attack(true, nullptr, mname);
    }

    if (player_has_confusing_attack())
    {
      monster_set_confused(tp);
      player_remove_confusing_attack();
      if (!player_is_blind())
      {
        io_msg("your hands stop glowing %s",
               player_is_hallucinating() ? color_random().c_str() : "red");
        io_msg("%s appears confused", mname);
      }
    }

    monster_start_running(monster_pos);
    return true;
  }
  monster_start_running(monster_pos);

  if (thrown && !to_death)
    fight_missile_miss(weapon, mname);
  else if (!to_death)
    print_attack(false, nullptr, mname);
  return false;
}

int
fight_against_player(Monster* mp)
{
  /* Since this is an attack, stop running and any healing that was
   * going on at the time */
  player_alerted = true;
  command_stop(false);
  daemon_reset_doctor(0);

  /* If we're fighting something to death and get backstabbed, return command */
  if (to_death && !monster_is_players_target(mp))
    to_death = false;

  /* If it's a xeroc, tag it as known */
  if (mp->t_type == 'X' && mp->t_disguise != 'X' && !player_is_blind()
      && !player_is_hallucinating())
    mp->t_disguise = 'X';

  char mname[MAXSTR];
  monster_name(mp, mname);

  if (roll_attacks(mp, nullptr, nullptr, false))
  {
    if (mp->t_type != 'I' && !to_death)
      print_attack(true, mname, nullptr);

    if (player_get_health() <= 0)
      death(mp->t_type);

    monster_do_special_ability(&mp);
    /* N.B! mp can be null after this point! */

  }

  else if (mp->t_type != 'I')
  {
    if (mp->t_type == 'F')
    {
      player_lose_health(monster_flytrap_hit);
      if (player_get_health() <= 0)
        death(mp->t_type);
    }

    if (!to_death)
      print_attack(false, mname, nullptr);
  }

  command_stop(false);
  io_refresh_statusline();
  return(mp == nullptr ? -1 : 0);
}

int
fight_swing_hits(int at_lvl, int op_arm, int wplus)
{
  int rand = os_rand_range(20);
  /* io_msg("%d + %d + %d vs %d", at_lvl, wplus, rand, op_arm); */
  return at_lvl + wplus + rand >= op_arm;
}

void
fight_missile_miss(Item const* weap, string const& mname)
{
  if (weap->o_type == WEAPON)
    io_msg("the %s misses %s", weapon_info[static_cast<size_t>(weap->o_which)].oi_name.c_str(), mname.c_str());
  else
    io_msg("you missed %s", mname.c_str());
}
