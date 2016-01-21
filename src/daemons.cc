#include <list>

#include "error_handling.h"
#include "game.h"
#include "io.h"
#include "pack.h"
#include "command.h"
#include "monster.h"
#include "rings.h"
#include "misc.h"
#include "player.h"
#include "level.h"
#include "options.h"
#include "os.h"
#include "rogue.h"

#include "daemons.h"

using namespace std;

struct Fuse {
    int type;
    daemon_function func;
    int time;
};

struct Daemon {
    int type;
    daemon_function func;
};

static list<Daemon> daemons;
static list<Fuse> fuses;

static int quiet_rounds = 0;

static void execute_daemon_function(daemon_function func) {
  switch(func) {
    case runners_move:          daemon_runners_move(); break;
    case doctor:                daemon_doctor(); break;
    case change_visuals:        daemon_change_visuals(); break;
    case ring_abilities:        daemon_ring_abilities(); break;
    case remove_true_sight:     player->remove_true_sight(); break;
    case set_not_confused:      player->set_not_confused(); break;
    case remove_sense_monsters: player->remove_sense_monsters(); break;
    case set_not_hallucinating: player->set_not_hallucinating(); break;
    case decrease_speed:        player->decrease_speed(); break;
    case set_not_blind:         player->set_not_blind(); break;
    case set_not_levitating:    player->set_not_levitating(); break;
  }
}

// Run all the daemons that are active with the current flag
static void daemon_run_all(int flag) {
  for (Daemon& daemon : daemons) {
    if (daemon.type == flag) {
      execute_daemon_function(daemon.func);
    }
  }
}

// Decrement counters and start needed fuses
static void daemon_run_fuses(int flag) {

  // List is not autoincrementing, so we can erase elements during the loop
  for (auto fuse = fuses.begin(); fuse != fuses.end();) {
    if (fuse->type == flag && --fuse->time <= 0) {
      execute_daemon_function(fuse->func);
      fuses.erase(fuse++);

    } else {
      fuse++;
    }
  }
}

void daemon_run_before() {
  daemon_run_all(BEFORE);
  daemon_run_fuses(BEFORE);
}

void daemon_run_after() {
  daemon_run_all(AFTER);
  daemon_run_fuses(AFTER);
}


// Start a daemon, takes a function.
void daemon_start(daemon_function func, int type) {
  daemons.push_back({type, func});
}

// Remove a daemon from the list
void daemon_kill(daemon_function func) {
  auto results = find_if(daemons.begin(), daemons.end(),
      [&func] (Daemon const& daemon) {
    return daemon.func == func;
  });

  if (results == daemons.end()) {
    error("Unable to find daemon to kill");
  }

  daemons.erase(results);
}


// Start a fuse to go off in a certain number of turns
void daemon_start_fuse(daemon_function func, int time, int type) {
  fuses.push_back({type, func, time});
}

// Increase the time until a fuse goes off */
void daemon_lengthen_fuse(daemon_function func, int xtime) {
  auto results = find_if(fuses.begin(), fuses.end(),
      [&func] (Fuse const& fuse) {
    return fuse.func == func;
  });

  if (results == fuses.end()) {
    error("Unable to find fuse to lengthen");
  }

  results->time += xtime;
}

// Put out a fuse
void daemon_extinguish_fuse(daemon_function func) {
  auto results = find_if(fuses.begin(), fuses.end(),
      [&func] (Fuse const& fuse) {
    return fuse.func == func;
  });

  if (results == fuses.end()) {
    error("Unable to find fuse to lengthen");
  }

  fuses.erase(results);
}


// Stop the daemon doctor from healing
void daemon_reset_doctor() {
  quiet_rounds = 0;
}

// A healing daemon that restors hit points after rest
void daemon_doctor() {
  int ohp = player->get_health();
  if (ohp == player->get_max_health()) {
    return;
  }

  quiet_rounds++;
  if (player->get_level() < 8) {
    if (quiet_rounds + (player->get_level() << 1) > 20) {
      player->restore_health(1, false);
    }
  }
  else if (quiet_rounds >= 3) {
    player->restore_health(os_rand_range(player->get_level() - 7) + 1, false);
  }

  for (int i = 0; i < PACK_RING_SLOTS; ++i) {
    Item *ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring != nullptr && ring->o_which == R_REGEN)
      player->restore_health(1, false);
  }

  if (ohp != player->get_health())
    quiet_rounds = 0;
}

// change the characters for the player
void daemon_change_visuals() {
  if (player->is_running() && jump) {
    return;
  }

  // change the items
  for (Item* tp : Game::level->items) {
    if (player->can_see(tp->get_pos())) {
      mvaddcch(tp->get_y(), tp->get_x(), static_cast<chtype>(rnd_thing()));
    }
  }

  // change the stairs
  if (player->has_seen_stairs()) {
    mvaddcch(Game::level->get_stairs_y(),
             Game::level->get_stairs_x(),
             static_cast<chtype>(rnd_thing()));
  }

  // change the monsters
  monster_show_all_as_trippy();
}

// Make all running monsters move
void daemon_runners_move() {
  monster_move_all();
}

void daemon_ring_abilities() {
  for (int i = 0; i < PACK_RING_SLOTS; ++i) {
    Item* obj = pack_equipped_item(pack_ring_slots[i]);
    if (obj == nullptr) {
      continue;

    } else if (obj->o_which == R_SEARCH) {
      player->search();
    } else if (obj->o_which == R_TELEPORT && os_rand_range(50) == 0) {
      player->teleport(nullptr);
    }
  }
}
