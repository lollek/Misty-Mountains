#pragma once

#include <istream>
#include <ostream>

namespace Daemons {

void init_daemons();
void save_daemons(std::ostream&);
void test_daemons();
void load_daemons(std::istream&);
void free_daemons();

enum daemon_function {
  runners_move,
  doctor,
  ring_abilities,
  remove_true_sight,
  set_not_confused,
  remove_sense_monsters,
  remove_sense_magic,
  decrease_speed,
  set_not_blind,
  set_not_levitating
};

struct Fuse {
  int type;
  Daemons::daemon_function func;
  int time;

  Fuse() = default;
  Fuse(int type, daemon_function func, int time);
  ~Fuse() = default;
  Fuse(Fuse const&) = default;
  Fuse& operator=(Fuse const&) = default;
  Fuse& operator=(Fuse&&) = default;

  bool operator==(Fuse const&) const;
  bool operator!=(Fuse const&) const;
};

struct Daemon {
  int type;
  Daemons::daemon_function func;

  Daemon() = default;
  Daemon(int type, daemon_function func);
  ~Daemon() = default;
  Daemon(Daemon const&) = default;
  Daemon& operator=(Daemon const&) = default;
  Daemon& operator=(Daemon&&) = default;

  bool operator==(Daemon const&) const;
  bool operator!=(Daemon const&) const;
};

// API
void daemon_run_before();
void daemon_run_after();

// Daemons
void daemon_start(daemon_function func, int type);
void daemon_kill(daemon_function func);

// Fuses
void daemon_start_fuse(daemon_function func, int time, int type);
void daemon_extinguish_fuse(daemon_function func);
void daemon_lengthen_fuse(daemon_function func, int xtime);

// Daemon actions
void daemon_doctor();
void daemon_change_visuals();
void daemon_runners_move();
void daemon_ring_abilities();

// Daemon action affectors
void daemon_reset_doctor();

}