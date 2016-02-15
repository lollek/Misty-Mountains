#pragma once

#include <fstream>

#include "player.h"

namespace Daemons {

void init_daemons();
void save_daemons(std::ofstream&);
void load_daemons(std::ifstream&);
void free_daemons();

enum daemon_function {
  runners_move,
  doctor,
  ring_abilities,
  remove_true_sight,
  set_not_confused,
  remove_sense_monsters,
  decrease_speed,
  set_not_blind,
  set_not_levitating
};

struct Fuse {
    int type;
    Daemons::daemon_function func;
    int time;
};

struct Daemon {
    int type;
    Daemons::daemon_function func;
};

/* API */
void daemon_run_before();
void daemon_run_after();

/* Daemons */
void daemon_start(daemon_function func, int type);
void daemon_kill(daemon_function func);

/* Fuses */
void daemon_start_fuse(daemon_function func, int time, int type);
void daemon_extinguish_fuse(daemon_function func);
void daemon_lengthen_fuse(daemon_function func, int xtime);

/* Daemon actions */
void daemon_doctor();
void daemon_change_visuals();
void daemon_runners_move();
void daemon_ring_abilities();

/* Daemon action affectors */
void daemon_reset_doctor();

}
