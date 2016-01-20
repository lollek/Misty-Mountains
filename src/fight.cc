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
#include "attack_modifier.h"

#include "fight.h"

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

// Calculate the damage an attacker does.
// Weapon can be null when no weapon was used
static attack_modifier
calculate_attacker(Character const& attacker, Item* weapon, bool thrown)
{
  attack_modifier mod;

  // Weapons override base attack (at the moment, monsters shouldn't use weapons)
  if (weapon != nullptr) {
    mod.to_hit = weapon->o_hplus;
    mod.to_dmg = weapon->o_dplus;

    if (thrown) {
      mod.damage.push_back(weapon->o_hurldmg);
    } else {
      mod.damage.push_back(weapon->o_damage);
    }

  // But otherwise we use the attacker's stats (can happen to both monsters and player)
  } else {
    mod.damage = attacker.get_attacks();
    if (mod.damage.empty()) {
      error("No damage was copied from attacker. Bad template?");
    }
  }

  mod.add_strength_modifiers(attacker.get_strength());

  if (&attacker == player) {

    add_ring_attack_modifiers(mod);
    if (thrown) {

      Item const* held_weapon = pack_equipped_item(EQUIPMENT_RHAND);

      // If the weapon was an arrow and player is holding the bow. Add
      // modifiers
      if (weapon->o_launch != NO_WEAPON && held_weapon != nullptr &&
          weapon->o_launch == held_weapon->o_which) {

          mod.damage.at(0)  = held_weapon->o_hurldmg;
          mod.to_hit       += held_weapon->o_hplus;
          mod.to_dmg       += held_weapon->o_dplus;
      }
    }

  // Venus Flytraps have a different kind of dmg system. It adds damage for
  // every successful hit
  } else if (attacker.get_type() == 'F') {
    mod.damage[0].sides = monster_flytrap_hit;
  }

  return mod;
}

// Roll attackers attack vs defenders defense and then take damage if it hits
static bool
roll_attacks(Character* attacker, Character* defender, Item* weapon, bool thrown) {

  if (attacker == nullptr) {
    error("Attacker was null");
  } else if (defender == nullptr) {
    error("Defender was null");
  }

  attack_modifier mod = calculate_attacker(*attacker, weapon, thrown);

  /* If defender is stuck in some way,the attacker gets a bonus to hit */
  if ((defender == player && player_turns_without_action)
      || defender->is_held()
      || defender->is_stuck()) {
    mod.to_hit += 4;
  }

  if (mod.damage.empty()) {
    error("No damage at all?");
  } else if (mod.damage.at(0).sides < 0 || mod.damage.at(0).dices < 0) {
    error("Damage dice was negative");
  }

  bool did_hit = false;
  for (damage const& dmg : mod.damage) {

    if (dmg.sides == 0 && dmg.dices == 0) {
      continue;
    }

    int defense = defender->get_armor();
    if (fight_swing_hits(attacker->get_level(), defense, mod.to_hit)) {

      int damage = roll(dmg.dices, dmg.sides) + mod.to_dmg;
      if (damage > 0) {
        defender->take_damage(damage);
      }
      did_hit = true;
    }
  }

  return did_hit;
}

// Print a message to indicate a hit or miss
static void
print_attack(bool hit, Character* attacker, Character* defender) {

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
  daemon_reset_doctor();

  /* Let him know it was really a xeroc (if it was one) */
  if (tp->get_type() == 'X' && tp->t_disguise != 'X' && !player->is_blind()) {

    tp->t_disguise = 'X';
    io_msg("wait!  That's a xeroc!");
    if (!thrown) {
      return false;
    }
  }

  if (roll_attacks(player, tp, weapon, thrown)) {

    if (tp->get_health() <= 0) {
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

    if (player->has_confusing_attack()) {

      tp->set_confused();
      player->remove_confusing_attack();
      if (!player->is_blind()) {

        io_msg("your hands stop glowing %s",
               player->is_hallucinating() ? color_random().c_str() : "red");
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
  daemon_reset_doctor();

  /* If we're fighting something to death and get backstabbed, return command */
  if (to_death && !mp->is_players_target()) {
    to_death = false;
  }

  /* If it's a xeroc, tag it as known */
  if (mp->get_type() == 'X' && mp->t_disguise != 'X' && !player->is_blind()
      && !player->is_hallucinating()) {
    mp->t_disguise = 'X';
  }

  if (roll_attacks(mp, player, nullptr, false)) {

    if (mp->get_type() != 'I' && !to_death) {
      print_attack(true, mp, player);
    }

    if (player->get_health() <= 0) {
      death(mp->get_type());
    }

    monster_do_special_ability(&mp);
    /* N.B! mp can be null after this point! */

  }

  else if (mp->get_type() != 'I') {

    if (mp->get_type() == 'F') {

      player->take_damage(monster_flytrap_hit);
      if (player->get_health() <= 0) {
        death(mp->get_type());
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
