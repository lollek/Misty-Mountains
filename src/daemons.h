#ifndef _ROGUE14_DAEMONS_H_
#define _ROGUE14_DAEMONS_H_

struct delayed_action {
    int d_type;
    void (*d_func)();
    int d_arg;
    int d_time;
};

void* __daemons_ptr(void);

/* API */
void daemon_run_before(void);
void daemon_run_after(void);

/* Daemons */
void daemon_start(void (*func)(), int arg, int type);
void daemon_kill(void (*func)());

/* Fuses */
void daemon_start_fuse(void (*func)(), int arg, int time, int type);
void daemon_extinguish_fuse(void (*func)());
void daemon_lengthen_fuse(void (*func)(), int xtime);

/* Daemon actions */
void daemon_doctor(void);
void daemon_start_wanderer(void);
void daemon_rollwand(void);
void daemon_digest_food(void);
void daemon_change_visuals(void);
void daemon_runners_move(void);
void daemon_ring_abilities(void);

/* Daemon action affectors */
void daemon_reset_doctor(void);

#endif /* _ROGUE14_DAEMONS_H_ */
