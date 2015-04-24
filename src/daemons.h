#ifndef _ROGUE14_DAEMONS_H_
#define _ROGUE14_DAEMONS_H_

struct delayed_action {
    int d_type;
    void (*d_func)();
    int d_arg;
    int d_time;
};

void *__daemons_ptr(void);

void daemon_start(void (*func)(), int arg, int type);
void daemon_kill(void (*func)());
void daemon_run_all(int flag);

void daemon_start_fuse(void (*func)(), int arg, int time, int type);
void daemon_extinguish_fuse(void (*func)());
void daemon_lengthen_fuse(void (*func)(), int xtime);
void daemon_run_fuses(int flag);

void daemon_doctor(void);
void daemon_reset_doctor(void);

#endif /* _ROGUE14_DAEMONS_H_ */
