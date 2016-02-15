#include <list>
#include <algorithm>

#include "disk.h"
#include "error_handling.h"
#include "game.h"
#include "io.h"
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

static list<Daemons::Daemon>* daemons = nullptr;
static list<Daemons::Fuse>*   fuses = nullptr;

void Daemons::init_daemons() {
  daemons = new list<Daemon>;
  fuses   = new list<Fuse>;
}

static unsigned long long constexpr TAG_DAEMONS   = 0x5000000000000000ULL;
static unsigned long long constexpr TAG_DAEMONLIST= 0x5000000000000001ULL;
static unsigned long long constexpr TAG_FUSELIST  = 0x5000000000000002ULL;

void Daemons::save_daemons(std::ofstream& data) {
  Disk::save_tag(TAG_DAEMONS, data);
  Disk::save(TAG_DAEMONLIST, daemons, data);
  Disk::save(TAG_FUSELIST, fuses, data);
}

void Daemons::load_daemons(std::ifstream& data) {
  if (!Disk::load_tag(TAG_DAEMONS, data))             { error("No daemons found"); }
  if (!Disk::load(TAG_DAEMONLIST, daemons, data))     { error("Daemon tag error 1"); }
  if (!Disk::load(TAG_FUSELIST, fuses, data))         { error("Daemon tag error 2"); }
}

void Daemons::free_daemons() {
  delete daemons;
  daemons = nullptr;

  delete fuses;
  fuses = nullptr;
}

static int quiet_rounds = 0;

static void execute_daemon_function(Daemons::daemon_function func) {
  switch(func) {
    case Daemons::runners_move:          Daemons::daemon_runners_move(); break;
    case Daemons::doctor:                Daemons::daemon_doctor(); break;
    case Daemons::ring_abilities:        Daemons::daemon_ring_abilities(); break;
    case Daemons::remove_true_sight:     player->remove_true_sight(); break;
    case Daemons::set_not_confused:      player->set_not_confused(); break;
    case Daemons::remove_sense_monsters: player->remove_sense_monsters(); break;
    case Daemons::decrease_speed:        player->decrease_speed(); break;
    case Daemons::set_not_blind:         player->set_not_blind(); break;
    case Daemons::set_not_levitating:    player->set_not_levitating(); break;
  }
}

// Run all the daemons that are active with the current flag
static void daemon_run_all(int flag) {
  for (Daemons::Daemon& daemon : *daemons) {
    if (daemon.type == flag) {
      execute_daemon_function(daemon.func);
    }
  }
}

// Decrement counters and start needed fuses
static void daemon_run_fuses(int flag) {

  // List is not autoincrementing, so we can erase elements during the loop
  for (auto fuse = fuses->begin(); fuse != fuses->end();) {
    if (fuse->type == flag && --fuse->time <= 0) {
      execute_daemon_function(fuse->func);
      fuses->erase(fuse++);

    } else {
      fuse++;
    }
  }
}

void Daemons::daemon_run_before() {
  daemon_run_all(BEFORE);
  daemon_run_fuses(BEFORE);
}

void Daemons::daemon_run_after() {
  daemon_run_all(AFTER);
  daemon_run_fuses(AFTER);
}


// Start a daemon, takes a function.
void Daemons::daemon_start(daemon_function func, int type) {
  daemons->push_back({type, func});
}

// Remove a daemon from the list
void Daemons::daemon_kill(daemon_function func) {
  auto results = find_if(daemons->begin(), daemons->end(),
      [&func] (Daemon const& daemon) {
    return daemon.func == func;
  });

  if (results == daemons->end()) {
    error("Unable to find daemon to kill");
  }

  daemons->erase(results);
}


// Start a fuse to go off in a certain number of turns
void Daemons::daemon_start_fuse(daemon_function func, int time, int type) {
  fuses->push_back({type, func, time});
}

// Increase the time until a fuse goes off */
void Daemons::daemon_lengthen_fuse(daemon_function func, int xtime) {
  auto results = find_if(fuses->begin(), fuses->end(),
      [&func] (Fuse const& fuse) {
    return fuse.func == func;
  });

  if (results == fuses->end()) {
    error("Unable to find fuse to lengthen");
  }

  results->time += xtime;
}

// Put out a fuse
void Daemons::daemon_extinguish_fuse(daemon_function func) {
  auto results = find_if(fuses->begin(), fuses->end(),
      [&func] (Fuse const& fuse) {
    return fuse.func == func;
  });

  if (results == fuses->end()) {
    error("Unable to find fuse to lengthen");
  }

  fuses->erase(results);
}


// Stop the daemon doctor from healing
void Daemons::daemon_reset_doctor() {
  quiet_rounds = 0;
}

// A healing daemon that restors hit points after rest
void Daemons::daemon_doctor() {
  int ohp = player->get_health();
  if (ohp == player->get_max_health()) {
    return;
  }

  int rings_of_regen = static_cast<int>(player->pack_num_items(IO::Ring, Ring::Regeneration));
  if (rings_of_regen > 0) {
    player->restore_health(rings_of_regen, false);
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


  if (ohp != player->get_health())
    quiet_rounds = 0;
}

// Make all running monsters move
void Daemons::daemon_runners_move() {
  Monster::all_move();
}

void Daemons::daemon_ring_abilities() {
  player->equipment_run_abilities();
}

