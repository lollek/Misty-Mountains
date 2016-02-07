#include <string>

#include "error_handling.h"
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

using namespace std;

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

static trap_t trap_door_player(void) {
  Game::new_level(Game::current_level + 1);
  Game::io->message("you fell into a trap!");
  return T_DOOR;
}

static trap_t trap_door_monster(Monster** victim_ptr) {
  Monster* victim = *victim_ptr;

  if (monster_seen_by_player(victim)) {
    stringstream os;
    os << victim->get_name()
       << " fell through the floor";
    Game::io->message(os.str());
  }

  monster_remove_from_screen(victim_ptr, false);
  return T_DOOR;
}

static enum trap_t trap_bear_player(void) {
  player->become_stuck();
  Game::io->message("you are caught in a bear trap");
  return T_BEAR;
}

static enum trap_t trap_bear_monster(Monster* victim) {
  if (monster_seen_by_player(victim)) {
    stringstream os;
    os << victim->get_name()
       << " was caught in a bear trap";
    Game::io->message(os.str());
  }
  victim->set_stuck();
  return T_BEAR;
}

static enum trap_t trap_myst_player(void) {
  stringstream os;
  switch(os_rand_range(11)) {
    case 0: os << "you are suddenly in a parallel dimension"; break;
    case 1: os << "the light in here suddenly seems " << Color::random(); break;
    case 2: os << "you feel a sting in the side of your neck"; break;
    case 3: os << "multi-colored lines swirl around you, then fade"; break;
    case 4: os << "a " << Color::random() << " light flashes in your eyes"; break;
    case 5: os << "a spike shoots past your ear!"; break;
    case 6: os << Color::random() << " sparks dance across your armor"; break;
    case 7: os << "you suddenly feel very thirsty"; break;
    case 8: os << "you feel time speed up suddenly"; break;
    case 9: os << "time now seems to be going slower"; break;
    case 10: os << "you pack turns " << Color::random() << "!"; break;
  }
  Game::io->message(os.str());
  return T_MYST;
}

static enum trap_t trap_myst_monster(Monster* victim) {
  if (monster_seen_by_player(victim)) {
    stringstream os;
    os << victim->get_name() << " seems to have stepped on something";
    Game::io->message(os.str());
  }
  return T_MYST;
}

static enum trap_t trap_sleep_player(void) {
  player->fall_asleep();
  Game::io->message("a strange white mist envelops you");
  return T_SLEEP;
}

static enum trap_t trap_sleep_monster(Monster* victim) {
  if (monster_seen_by_player(victim)) {
    stringstream os;
    os << victim->get_name() << " collapsed to the ground";
    Game::io->message(os.str());
  }
  victim->set_held();
  return T_SLEEP;
}

static enum trap_t trap_arrow_player(void) {
  if (fight_swing_hits(player->get_level() - 1, player->get_armor(), 1)) {
    player->take_damage(roll(1, 6));
    if (player->get_health() <= 0) {
      Game::io->message("an arrow killed you");
      death(DEATH_ARROW);

    } else {
      Game::io->message("oh no! An arrow shot you");
    }

  } else {
    Item* arrow = new Weapon(Weapon::ARROW, false);
    arrow->o_count = 1;
    arrow->set_position(player->get_position());
    weapon_missile_fall(arrow, false);
    Game::io->message("an arrow shoots past you");
  }
  return T_ARROW;
}

static enum trap_t trap_arrow_monster(Monster** victim_ptr) {
  Monster* victim = *victim_ptr;

  if (fight_swing_hits(victim->get_level() -1,
        victim->get_armor(), 1)) {

    victim->take_damage(roll(1,6));

    if (victim->get_health() <= 0) {
      if (monster_seen_by_player(victim)) {
        Game::io->message("An arrow killed " +  victim->get_name());
      }
      monster_on_death(victim_ptr, false);
      victim = nullptr;

    } else if (monster_seen_by_player(victim)) {
      Game::io->message("An arrow shot " +  victim->get_name());
    }

  } else {
    Item* arrow = new Weapon(Weapon::ARROW, false);
    arrow->o_count = 1;
    arrow->set_position(victim->get_position());
    weapon_missile_fall(arrow, false);
    if (monster_seen_by_player(victim)) {
      Game::io->message("An arrow barely missed " + victim->get_name());
    }
  }
  return T_ARROW;
}

static enum trap_t trap_telep_player(Coordinate const* trap_coord) {
  player->teleport(nullptr);
  // Mark trap before we leave
  Game::io->print_color(trap_coord->x, trap_coord->y, TRAP);
  return T_TELEP;
}

static enum trap_t trap_telep_monster(Monster* victim) {
  stringstream os;

  bool was_seen = monster_seen_by_player(victim);
  if (was_seen) {
    os << victim->get_name();
  }

  monster_teleport(victim, nullptr);
  if (was_seen) {
    if (monster_seen_by_player(victim)) {
      os << " teleported a short distance";
    } else {
      os << " disappeared";
    }
  }

  if (!was_seen && monster_seen_by_player(victim)) {
    os << victim->get_name()
       << " appeared out of thin air";
  }

  Game::io->message(os.str());
  return T_TELEP;
}

static enum trap_t trap_dart_player(void) {
  if (!fight_swing_hits(player->get_level() + 1, player->get_armor(), 1)) {
    Game::io->message("a small dart whizzes by your ear and vanishes");

  } else {
    player->take_damage(roll(1, 4));
    if (player->get_health() <= 0) {
      Game::io->message("a poisoned dart killed you");
      death(DEATH_DART);
    }

    if (!player->has_ring_with_ability(Ring::Type::SUSTSTR) &&
        !player->saving_throw(VS_POISON)) {
      player->modify_strength(-1);
    }

    Game::io->message("a small dart just hit you in the shoulder");
  }
  return T_DART;
}

static enum trap_t trap_dart_monster(Monster** victim_ptr) {
  Monster* victim = *victim_ptr;

  /* TODO: In the future this should probably weaken the monster */
  if (fight_swing_hits(victim->get_level() + 1,
        victim->get_armor(), 1)) {
    victim->take_damage(roll(1,4));

    if (victim->get_health() <= 0) {
      if (monster_seen_by_player(victim)) {
        Game::io->message("A poisoned dart killed " + victim->get_name());
      }
      monster_on_death(victim_ptr, false);
      victim = nullptr;

    } else if (monster_seen_by_player(victim)) {
      Game::io->message("An dart hit " + victim->get_name());
    }

  } else if (monster_seen_by_player(victim)) {
    Game::io->message("A dart barely missed " + victim->get_name());
  }
  return T_DART;
}

static enum trap_t trap_rust_player(void) {
  Game::io->message("a gush of water hits you on the head");
  player->rust_armor();
  return T_RUST;
}

static enum trap_t trap_rust_monster(Monster* victim) {
  if (monster_seen_by_player(victim)) {
    Game::io->message("a gush of water hits " + victim->get_name());
  }
  return T_RUST;
}

trap_t trap_player(Coordinate const& trap_coord) {
  command_stop(true);

  Game::level->set_ch(trap_coord, TRAP);
  Game::level->set_discovered(trap_coord);

  switch (Game::level->get_trap_type(trap_coord)) {
    case T_DOOR:  return trap_door_player();
    case T_BEAR:  return trap_bear_player();
    case T_MYST:  return trap_myst_player();
    case T_SLEEP: return trap_sleep_player();
    case T_ARROW: return trap_arrow_player();
    case T_TELEP: return trap_telep_player(&trap_coord);
    case T_DART:  return trap_dart_player();
    case T_RUST:  return trap_rust_player();

    default: error("Unknown trap type triggered");
  }
}

trap_t trap_spring(Monster** victim, Coordinate const& trap_coord) {
  if (victim == nullptr) {
    error("victim = nullptr");
  } else if (*victim == nullptr) {
    error("*victim = nullptr");
  }

  if (monster_seen_by_player(*victim)) {
    Game::level->set_ch(trap_coord, TRAP);
    Game::level->set_discovered(trap_coord);
  }

  switch (Game::level->get_trap_type(trap_coord)) {
    case T_DOOR:  return trap_door_monster(victim);
    case T_BEAR:  return trap_bear_monster(*victim);
    case T_MYST:  return trap_myst_monster(*victim);
    case T_SLEEP: return trap_sleep_monster(*victim);
    case T_ARROW: return trap_arrow_monster(victim);
    case T_TELEP: return trap_telep_monster(*victim);
    case T_DART:  return trap_dart_monster(victim);
    case T_RUST:  return trap_rust_monster(*victim);

    default: error("Unknown trap type triggered");
  }
}


