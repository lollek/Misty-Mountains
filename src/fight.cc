#include <string>
#include <vector>
#include <sstream>

using namespace std;

#include "error_handling.h"
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

struct attack_modifier {
  int    to_hit;
  int    to_dmg;
  damage damage[MAXATTACKS];
};

// Add item bonuses to damage and to-hit
static void
add_ring_attack_modifiers(attack_modifier& mod) {

  for (int i = 0; i < PACK_RING_SLOTS; ++i) {

    Item const* ring_thing = pack_equipped_item(pack_ring_slots[i]);
    if (ring_thing == nullptr) {
      continue;
    }

    Item const* ring = ring_thing;

    if (ring->o_which == R_ADDDAM) {
      mod.to_dmg += ring->o_arm;
    } else if (ring->o_which == R_ADDHIT) {
      mod.to_dmg += ring->o_arm;
    }
  }
}

static void
add_strength_attack_modifiers(int strength, attack_modifier& mod) {
  switch (strength) {

    case  0: mod.to_hit -= 7; mod.to_dmg -= 7; return;
    case  1: mod.to_hit -= 6; mod.to_dmg -= 6; return;
    case  2: mod.to_hit -= 5; mod.to_dmg -= 5; return;
    case  3: mod.to_hit -= 4; mod.to_dmg -= 4; return;
    case  4: mod.to_hit -= 3; mod.to_dmg -= 3; return;
    case  5: mod.to_hit -= 2; mod.to_dmg -= 2; return;
    case  6: mod.to_hit -= 1; mod.to_dmg -= 1; return;

    default: return;

    case 16:                  mod.to_dmg += 1; return;
    case 17: mod.to_hit += 1; mod.to_dmg += 1; return;
    case 18: mod.to_hit += 1; mod.to_dmg += 2; return;
    case 19: mod.to_hit += 1; mod.to_dmg += 3; return;
    case 20: mod.to_hit += 1; mod.to_dmg += 3; return;
    case 21: mod.to_hit += 2; mod.to_dmg += 4; return;
    case 22: mod.to_hit += 2; mod.to_dmg += 5; return;
    case 23: mod.to_hit += 2; mod.to_dmg += 5; return;
    case 24: mod.to_hit += 2; mod.to_dmg += 5; return;
    case 25: mod.to_hit += 2; mod.to_dmg += 5; return;
    case 26: mod.to_hit += 2; mod.to_dmg += 5; return;
    case 27: mod.to_hit += 2; mod.to_dmg += 5; return;
    case 28: mod.to_hit += 2; mod.to_dmg += 5; return;
    case 29: mod.to_hit += 2; mod.to_dmg += 5; return;
    case 30: mod.to_hit += 2; mod.to_dmg += 5; return;
    case 31: mod.to_hit += 3; mod.to_dmg += 6; return;
  }
}

static void
calculate_attacker(Monster* attacker, Item* weapon, bool thrown, attack_modifier& mod)
{
  if (weapon != nullptr) {
    mod.damage[0] = weapon->o_damage;
    mod.to_hit += weapon->o_hplus;
    mod.to_dmg += weapon->o_dplus;
  }

  add_strength_attack_modifiers(attacker->t_stats.s_str, mod);

  /* Player stuff */
  if (attacker == player) {

    add_ring_attack_modifiers(mod);
    if (thrown) {

      Item const* held_weapon = pack_equipped_item(EQUIPMENT_RHAND);
      if ((weapon->o_flags & ISMISL) && held_weapon != nullptr
          && held_weapon->o_which == weapon->o_launch) {

        mod.damage[0] = weapon->o_hurldmg;
        mod.to_hit += held_weapon->o_hplus;
        mod.to_dmg += held_weapon->o_dplus;

      } else if (weapon->o_launch == -1) {
        mod.damage[0] = weapon->o_hurldmg;
      }
    }

  /* Venus Flytraps have a different kind of dmg system */
  } else if (attacker->t_type == 'F') {
    mod.damage[0].sides = monster_flytrap_hit;
  }
}

// Roll attackers attack vs defenders defense and then take damage if it hits
static bool
roll_attacks(Monster* attacker, Monster* defender, Item* weapon, bool thrown) {

  if (attacker == nullptr) {
    error("Attacker was null");
  } else if (defender == nullptr) {
    error("Defender was null");
  }

  struct attack_modifier mod;
  mod.to_hit = 0;
  mod.to_dmg = 0;
  if (sizeof(mod.damage) != sizeof(attacker->t_stats.s_dmg)) {
    error("Size was not correct");
  }

  memcpy(mod.damage, attacker->t_stats.s_dmg, sizeof(attacker->t_stats.s_dmg));

  calculate_attacker(attacker, weapon, thrown, mod);

  /* If defender is stuck in some way,the attacker gets a bonus to hit */
  if ((defender == player && player_turns_without_action)
      || monster_is_held(defender)
      || monster_is_stuck(defender)) {
    mod.to_hit += 4;
  }

  if (mod.damage[0].sides == -1 || mod.damage[0].dices == -1) {
    error("Damage dice was negative");
  }

  bool did_hit = false;
  for (int i = 0; i < MAXATTACKS; ++i) {

    int dice_sides = mod.damage[i].sides;
    int dices = mod.damage[i].dices;

    if (dice_sides == 0 && dices == 0) {
      continue;
    }

    int defense = defender->get_armor();
    if (fight_swing_hits(attacker->t_stats.s_lvl, defense, mod.to_hit)) {

      int damage = roll(dices, dice_sides) + mod.to_dmg;
      if (damage > 0) {
        defender->t_stats.s_hpt -= damage;
      }
      did_hit = true;
    }
  }

  return did_hit;
}

// Print a message to indicate a hit or miss
static void
print_attack(bool hit, Monster* attacker, Monster* defender) {

  stringstream ss;
  ss << attacker->get_name() << " "
     << attacker->get_attack_string(hit) << " "
     << defender->get_name();
  io_msg("%s", ss.str().c_str());
}

int
fight_against_monster(Coordinate const* monster_pos, Item* weapon, bool thrown) {

  if (monster_pos == nullptr) {
    error("monster_pos was null");
  }

  Monster* tp = Game::level->get_monster(*monster_pos);
  if (tp == nullptr) {
    return !io_fail("fight_against_monster(%p, %p, %b) nullptr monster\r\n",
                 monster_pos, weapon, thrown);
  }

  /* Since we are fighting, things are not quiet so no healing takes place */
  command_stop(false);
  daemon_reset_doctor(0);

  /* Let him know it was really a xeroc (if it was one) */
  if (tp->t_type == 'X' && tp->t_disguise != 'X' && !player_is_blind()) {

    tp->t_disguise = 'X';
    io_msg("wait!  That's a xeroc!");
    if (!thrown) {
      return false;
    }
  }

  if (roll_attacks(player, tp, weapon, thrown)) {

    if (tp->t_stats.s_hpt <= 0) {
      monster_on_death(tp, true);
      return true;
    }

    if (!to_death) {

      if (thrown) {
        if (weapon->o_type == WEAPON) {
          io_msg_add("the %s hits ", weapon_info[static_cast<size_t>(weapon->o_which)].oi_name.c_str());
        } else {
          io_msg_add("you hit ");
        }
        io_msg("%s", tp->get_name().c_str());

      } else {
        print_attack(true, player, tp);
      }
    }

    if (player_has_confusing_attack()) {

      monster_set_confused(tp);
      player_remove_confusing_attack();
      if (!player_is_blind()) {

        io_msg("your hands stop glowing %s",
               player_is_hallucinating() ? color_random().c_str() : "red");
        io_msg("%s appears confused", tp->get_name().c_str());
      }
    }

    monster_start_running(monster_pos);
    return true;
  }
  monster_start_running(monster_pos);

  if (thrown && !to_death) {
    fight_missile_miss(weapon, tp->get_name().c_str());
  } else if (!to_death) {
    print_attack(false, player, tp);
  }
  return false;
}

int
fight_against_player(Monster* mp) {

  /* Since this is an attack, stop running and any healing that was
   * going on at the time */
  player_alerted = true;
  command_stop(false);
  daemon_reset_doctor(0);

  /* If we're fighting something to death and get backstabbed, return command */
  if (to_death && !monster_is_players_target(mp)) {
    to_death = false;
  }

  /* If it's a xeroc, tag it as known */
  if (mp->t_type == 'X' && mp->t_disguise != 'X' && !player_is_blind()
      && !player_is_hallucinating()) {
    mp->t_disguise = 'X';
  }

  if (roll_attacks(mp, player, nullptr, false)) {

    if (mp->t_type != 'I' && !to_death) {
      print_attack(true, mp, player);
    }

    if (player_get_health() <= 0) {
      death(mp->t_type);
    }

    monster_do_special_ability(&mp);
    /* N.B! mp can be null after this point! */

  }

  else if (mp->t_type != 'I') {

    if (mp->t_type == 'F') {

      player_lose_health(monster_flytrap_hit);
      if (player_get_health() <= 0) {
        death(mp->t_type);
      }
    }

    if (!to_death) {
      print_attack(false, mp, player);
    }
  }

  command_stop(false);
  io_refresh_statusline();
  return(mp == nullptr ? -1 : 0);
}

int
fight_swing_hits(int at_lvl, int op_arm, int wplus) {

  int rand = os_rand_range(20);
  /* io_msg("%d + %d + %d vs %d", at_lvl, wplus, rand, op_arm); */
  return at_lvl + wplus + rand >= op_arm;
}

void
fight_missile_miss(Item const* weap, string const& mname) {

  if (weap->o_type == WEAPON) {
    io_msg("the %s misses %s", weapon_info[static_cast<size_t>(weap->o_which)].oi_name.c_str(), mname.c_str());
  } else {
    io_msg("you missed %s", mname.c_str());
  }
}
