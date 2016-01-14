#ifndef ROGUE14_DAEMONS_H
#define ROGUE14_DAEMONS_H

struct delayed_action {
    int d_type;
    void (*d_func)(int);
    int d_arg;
    int d_time;
};

void* __daemons_ptr();

/* API */
void daemon_run_before();
void daemon_run_after();

/* Daemons */
void daemon_start(void (*func)(int), int arg, int type);
void daemon_kill(void (*func)(int));

/* Fuses */
void daemon_start_fuse(void (*func)(int), int arg, int time, int type);
void daemon_extinguish_fuse(void (*func)(int));
void daemon_lengthen_fuse(void (*func)(int), int xtime);

/* Daemon actions */
void daemon_doctor(int unused);
void daemon_start_wanderer(int unused);
void daemon_rollwand(int unused);
void daemon_change_visuals(int unused);
void daemon_runners_move(int unused);
void daemon_ring_abilities(int unused);

/* Daemon action affectors */
void daemon_reset_doctor(int unused);

#endif /* ROGUE14_DAEMONS_H */
