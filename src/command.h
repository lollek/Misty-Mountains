#ifndef _ROGUE14_COMMAND_H_
#define _ROGUE14_COMMAND_H_

#include <stdbool.h>

int command(void); /* Processes the user commands */
bool command_stop(bool stop_fighting);

void command_signal_endit(int sig); /* Exit the program abnormally  */
void command_signal_quit(int sig);  /* Have player make certain, then exit */
void command_signal_leave(int sig); /* Leave quickly, but curteously */


#endif /* _ROGUE14_COMMAND_H_ */
