#ifndef _ROGUE14_DAEMONS_H_
#define _ROGUE14_DAEMONS_H_

struct delayed_action {
    int d_type;
    void (*d_func)();
    int d_arg;
    int d_time;
};

void *__daemons_ptr(void);

#endif /* _ROGUE14_DAEMONS_H_ */
