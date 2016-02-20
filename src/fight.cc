#include <string>
#include <vector>
#include <sstream>

#include "wizard.h"
#include "error_handling.h"
#include "game.h"
#include "colors.h"
#include "command.h"
#include "io.h"
#include "armor.h"
#include "daemons.h"
#include "monster.h"
#include "rings.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "weapons.h"
#include "death.h"
#include "options.h"
#include "rogue.h"
#include "os.h"
#include "attack_modifier.h"

#include "fight.h"

using namespace std;

// Calculate the damage an attacker does.
// Weapon can be null when no weapon was used
static attack_modifier
calculate_attacker(Character const& attacker, Item* weapon, bool thrown)
{
  attack_modifier mod;

  // Weapons override base attack (at the moment, monsters shouldn't use weapons)
  if (weapon != nullptr) {
    mod.to_hit = weapon->get_hit_plus();
    mod.to_dmg = weapon->get_damage_plus();

    if (thrown) {
      mod.damage.push_back(weapon->get_throw_damage());

      class Weapon* bow = player->equipped_weapon();
      class Weapon* w_weapon = dynamic_cast<class Weapon*>(weapon);
      if (bow != nullptr && w_weapon != nullptr &&
          w_weapon->get_ammo_type() != Weapon::AmmoType::None &&
          bow->get_ammo_used() == w_weapon->get_ammo_type()) {
        int multiplier = bow->get_ammo_multiplier();
        for (damage& dmg : mod.damage) {
          dmg.dices *= multiplier;
        }
        mod.to_hit       += bow->get_hit_plus();
        mod.to_dmg       += bow->get_damage_plus();
      }
    } else {
      mod.damage.push_back(weapon->get_attack_damage());
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
    mod.to_dmg += player->pack_get_ring_modifier(Ring::Damage);
    mod.to_hit += player->pack_get_ring_modifier(Ring::Accuracy);
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

      if (wizard_dicerolls) {
        stringstream os;
        os << dmg.dices << "d" << dmg.sides << " + " << mod.to_dmg;
        Game::io->message(os.str());
      }

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
  Game::io->message(ss.str());
}

int
fight_against_monster(Coordinate const* monster_pos, Item* weapon, bool thrown,
                      string const* name_override) {

  if (monster_pos == nullptr) {
    error("monster_pos was null");
  }

  Monster* tp = Game::level->get_monster(*monster_pos);
  if (tp == nullptr) {
    error("No monster on pos");
  }

  /* Since we are fighting, things are not quiet so no healing takes place */
  command_stop(false);
  Daemons::daemon_reset_doctor();

  /* Let him know it was really a xeroc (if it was one) */
  if (!player->is_blind() && tp->get_subtype() == Monster::Xeroc &&
      tp->get_disguise() != tp->get_type()) {

    tp->set_disguise(static_cast<char>(tp->get_type()));
    Game::io->message("wait!  That's a xeroc!");
    if (!thrown) {
      return false;
    }
  }

  if (roll_attacks(player, tp, weapon, thrown)) {

    if (tp->get_health() <= 0) {
      monster_on_death(&tp, true);
      return true;
    }

    if (!to_death) {

      if (thrown) {
        if (weapon->o_type == IO::Weapon) {
          stringstream os;
          os
            << "the "
            << (name_override == nullptr
                ? Weapon::name(static_cast<Weapon::Type>(weapon->o_which))
                : *name_override)
            << " hits "
            << tp->get_name();
          Game::io->message(os.str());

        } else {
          stringstream os;
          os
            << "you hit "
            << tp->get_name();
          Game::io->message(os.str());
        }

      } else {
        print_attack(true, player, tp);
      }
    }

    if (player->has_confusing_attack()) {

      tp->set_confused();
      player->remove_confusing_attack();
      if (!player->is_blind()) {

        Game::io->message("your hands stop glowing red");
        Game::io->message(tp->get_name() + " appears confused");
      }
    }

    monster_start_running(monster_pos);
    return true;
  }
  monster_start_running(monster_pos);

  if (thrown && !to_death) {
    fight_missile_miss(weapon, tp->get_name().c_str(), name_override);
  } else if (!to_death) {
    print_attack(false, player, tp);
  }
  return false;
}

int
fight_against_player(Monster* mp) {

  // Since this is an attack, stop running and any healing that was
  // going on at the time
  player_alerted = true;
  command_stop(false);
  Daemons::daemon_reset_doctor();

  // Stop fighting to death if we are backstabbed
  if (to_death && !mp->is_players_target()) {
    to_death = false;
  }

  // If it's a xeroc, tag it as known
  if (!player->is_blind() && mp->get_subtype() == Monster::Xeroc &&
      mp->get_disguise() != mp->get_type()) {
    mp->set_disguise('X');
  }

  if (roll_attacks(mp, player, nullptr, false)) {
    // Monster hit player, and probably delt damage

    // berzerking causes to much text
    if (!to_death) {
      print_attack(true, mp, player);
    }

    // Check player death (doesn't return)
    if (player->get_health() <= 0) {
      death(mp->get_subtype());
    }

    // Do monster ability (mp can be null after this!)
    monster_do_special_ability(&mp);

  } else if (!to_death) {
    print_attack(false, mp, player);
  }

  command_stop(false);
  Game::io->refresh();
  return(mp == nullptr ? -1 : 0);
}

int
fight_swing_hits(int at_lvl, int op_arm, int wplus) {

  int rand = os_rand_range(20) + 1;

  if (wizard_dicerolls) {
    stringstream os;
    os << at_lvl << " + " << wplus << " + " << rand << " vs " << op_arm;
    Game::io->message(os.str());
  }

  // Forced miss
  if (rand == 1) {
    return false;

  // Forced hit
  } else if (rand == 20) {
    return true;

  } else {
    return at_lvl + wplus + rand >= op_arm;
  }
}

void
fight_missile_miss(Item const* weap, string const& mname,
                   string const* name_override) {

  if (weap->o_type == IO::Weapon) {
    if (name_override == nullptr) {
      Game::io->message("the " + Weapon::name(static_cast<Weapon::Type>(weap->o_which)) +
                        " misses " + mname);
    } else {
      Game::io->message("the " + *name_override +
                        " misses " + mname);
    }
  } else {
    Game::io->message("you missed " + mname);
  }
}
