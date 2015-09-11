#ifndef ROGUE14_SAVE_H
#define ROGUE14_SAVE_H

#include <stdbool.h>

extern char save_file_name[MAXSTR];

bool save_game(void);

/* Automatically save a file.  This is used if a HUP signal is recieved */
void save_auto(int signal) __attribute__((noreturn));

#endif /* ROGUE14_SAVE_H */
