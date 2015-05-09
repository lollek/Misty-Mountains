#ifndef _ROGUE14_COMMAND_H_
#define _ROGUE14_COMMAND_H_

#include <stdbool.h>

int command(void); /* Processes the user commands */

void command_stop(bool stop_fighting);


/* Exit the program abnormally.  */
void command_signal_endit(int sig);

/* Have player make certain, then exit.  */
void command_signal_quit(int sig);

/* Leave quickly, but curteously */
void command_signal_leave(int sig);


#endif /* _ROGUE14_COMMAND_H_ */
