#include <string>
#include <sstream>

#include "error_handling.h"
#include "game.h"
#include "coordinate.h"
#include "command.h"
#include "io.h"
#include "item/armor.h"
#include "fight.h"
#include "colors.h"
#include "level.h"
#include "item/rings.h"
#include "misc.h"
#include "item/weapons.h"
#include "monster.h"
#include "os.h"
#include "player.h"
#include "death.h"
#include "rogue.h"

#include "traps.h"

using namespace std;

static vector<string> const* trap_names = nullptr;

void Trap::init_traps() {
  trap_names = new vector<string> const {
    "a trapdoor",
    "an arrow trap",
    "a sleeping gas trap",
    "a beartrap",
    "a teleport trap",
    "a poison dart trap",
    "a rust trap",
    "a mysterious trap"
  };
}

void Trap::free_traps() {
  delete trap_names;
  trap_names = nullptr;
}

string Trap::name(Type type) {
  return trap_names->at(static_cast<size_t>(type));
}

static Trap::Type trap_door_player(void) {
  Game::new_level(Game::current_level + 1);
  Game::io->message("you fell into a trap!");
  return Trap::Door;
}

static Trap::Type trap_door_monster(Monster** victim_ptr) {
  Monster* victim = *victim_ptr;

  if (player->can_see(*victim)) {
    stringstream os;
    os << victim->get_name()
       << " fell through the floor";
    Game::io->message(os.str());
  }

  monster_remove_from_screen(victim_ptr, false);
  return Trap::Door;
}

static Trap::Type trap_bear_player(void) {
  player->become_stuck();
  Game::io->message("you are caught in a bear trap");
  return Trap::Beartrap;
}

static Trap::Type trap_bear_monster(Monster* victim) {
  if (player->can_see(*victim)) {
    stringstream os;
    os << victim->get_name()
       << " was caught in a bear trap";
    Game::io->message(os.str());
  }
  victim->set_stuck();
  return Trap::Beartrap;
}

static Trap::Type trap_myst_player(void) {
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
  return Trap::Mystery;
}

static Trap::Type trap_myst_monster(Monster* victim) {
  if (player->can_see(*victim)) {
    stringstream os;
    os << victim->get_name() << " seems to have stepped on something";
    Game::io->message(os.str());
  }
  return Trap::Mystery;
}

static Trap::Type trap_sleep_player(void) {
  player->fall_asleep();
  Game::io->message("a strange white mist envelops you");
  return Trap::Sleep;
}

static Trap::Type trap_sleep_monster(Monster* victim) {
  if (player->can_see(*victim)) {
    stringstream os;
    os << victim->get_name() << " collapsed to the ground";
    Game::io->message(os.str());
  }
  victim->set_held();
  return Trap::Sleep;
}

static Trap::Type trap_arrow_player(void) {
  if (fight_swing_hits(player->get_level() - 1, player->get_ac(), 1)) {
    player->take_damage(roll(1, 6));
    if (player->get_health() <= 0) {
      Game::io->message("an arrow killed you");
      death(DEATH_ARROW);

    } else {
      Game::io->message("oh no! An arrow shot you");
    }

  } else {
    Item* arrow = new class Weapon(Weapon::Arrow, false);
    arrow->o_count = 1;
    arrow->set_position(player->get_position());
    weapon_missile_fall(arrow, false);
    Game::io->message("an arrow shoots past you");
  }
  return Trap::Arrow;
}

static Trap::Type trap_arrow_monster(Monster** victim_ptr) {
  Monster* victim = *victim_ptr;

  if (fight_swing_hits(victim->get_level() -1,
        victim->get_ac(), 1)) {

    victim->take_damage(roll(1,6));

    if (victim->get_health() <= 0) {
      if (player->can_see(*victim)) {
        Game::io->message("An arrow killed " +  victim->get_name());
      }
      monster_on_death(victim_ptr, false);
      victim = nullptr;

    } else if (player->can_see(*victim)) {
      Game::io->message("An arrow shot " +  victim->get_name());
    }

  } else {
    Item* arrow = new class Weapon(Weapon::Arrow, false);
    arrow->o_count = 1;
    arrow->set_position(victim->get_position());
    weapon_missile_fall(arrow, false);
    if (player->can_see(*victim)) {
      Game::io->message("An arrow barely missed " + victim->get_name());
    }
  }
  return Trap::Arrow;
}

static Trap::Type trap_telep_player() {
  player->teleport(nullptr);
  return Trap::Teleport;
}

static Trap::Type trap_telep_monster(Monster* victim) {
  stringstream os;

  bool was_seen = player->can_see(*victim);
  if (was_seen) {
    os << victim->get_name();
  }

  monster_teleport(victim, nullptr);
  if (was_seen) {
    if (player->can_see(*victim)) {
      os << " teleported a short distance";
    } else {
      os << " disappeared";
    }
  }

  if (!was_seen && player->can_see(*victim)) {
    os << victim->get_name()
       << " appeared out of thin air";
  }

  Game::io->message(os.str());
  return Trap::Teleport;
}

static Trap::Type trap_dart_player(void) {
  if (!fight_swing_hits(player->get_level() + 1, player->get_ac(), 1)) {
    Game::io->message("a small dart whizzes by your ear and vanishes");

  } else {
    player->take_damage(roll(1, 4));
    if (player->get_health() <= 0) {
      Game::io->message("a poisoned dart killed you");
      death(DEATH_DART);
    }

    if (!player->has_ring_with_ability(Ring::SustainStrenght) &&
        !player->saving_throw(VS_POISON)) {
      player->modify_strength(-1);
    }

    Game::io->message("a small dart just hit you in the shoulder");
  }
  return Trap::Dart;
}

static Trap::Type trap_dart_monster(Monster** victim_ptr) {
  Monster* victim = *victim_ptr;

  /* TODO: In the future this should probably weaken the monster */
  if (fight_swing_hits(victim->get_level() + 1,
        victim->get_ac(), 1)) {
    victim->take_damage(roll(1,4));

    if (victim->get_health() <= 0) {
      if (player->can_see(*victim)) {
        Game::io->message("A poisoned dart killed " + victim->get_name());
      }
      monster_on_death(victim_ptr, false);
      victim = nullptr;

    } else if (player->can_see(*victim)) {
      Game::io->message("An dart hit " + victim->get_name());
    }

  } else if (player->can_see(*victim)) {
    Game::io->message("A dart barely missed " + victim->get_name());
  }
  return Trap::Dart;
}

static Trap::Type trap_rust_player(void) {
  Game::io->message("a gush of water hits you on the head");
  player->rust_armor();
  return Trap::Rust;
}

static Trap::Type trap_rust_monster(Monster* victim) {
  if (player->can_see(*victim)) {
    Game::io->message("a gush of water hits " + victim->get_name());
  }
  return Trap::Rust;
}

Trap::Type Trap::player(Coordinate const& trap_coord) {
  command_stop(true);

  Game::level->set_tile(trap_coord, Tile::Trap);
  Game::level->set_discovered(trap_coord);

  Trap::Type trap = Game::level->get_trap_type(trap_coord);
  if (trap == Trap::NTRAPS) {
    Game::level->set_trap_type(trap_coord, random());
  }

  switch (Game::level->get_trap_type(trap_coord)) {
    case Door:      return trap_door_player();
    case Beartrap:  return trap_bear_player();
    case Mystery:   return trap_myst_player();
    case Sleep:     return trap_sleep_player();
    case Arrow:     return trap_arrow_player();
    case Teleport:  return trap_telep_player();
    case Dart:      return trap_dart_player();
    case Rust:      return trap_rust_player();

    case NTRAPS: error("Unknown trap type triggered");
  }
}

Trap::Type Trap::spring(Monster** victim, Trap::Type trap_type) {
  if (victim == nullptr || *victim == nullptr) {
    error("null");
  }

  switch (trap_type) {
    case Door:     return trap_door_monster(victim);
    case Beartrap: return trap_bear_monster(*victim);
    case Mystery:  return trap_myst_monster(*victim);
    case Sleep:    return trap_sleep_monster(*victim);
    case Arrow:    return trap_arrow_monster(victim);
    case Teleport: return trap_telep_monster(*victim);
    case Dart:     return trap_dart_monster(victim);
    case Rust:     return trap_rust_monster(*victim);

    case NTRAPS: error("Unknown trap type triggered");
  }
}

Trap::Type Trap::random() {
  return static_cast<Trap::Type>(os_rand_range(NTRAPS));
}
