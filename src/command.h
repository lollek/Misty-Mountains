#pragma once

int command(); /* Processes the user commands */
bool command_stop(bool stop_fighting);

void command_signal_quit(int sig); /* Have player make certain, then exit */

/* Exit the program abnormally  */
void command_signal_endit(int sig) __attribute__((noreturn));

/* Leave quickly, but curteously */
void command_signal_leave(int sig) __attribute__((noreturn));
