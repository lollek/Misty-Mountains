#ifndef _ROGUE14_SAVE_H_
#define _ROGUE14_SAVE_H_

#include <stdbool.h>

extern char save_file_name[MAXSTR];

bool save_game(void);

/* Automatically save a file.  This is used if a HUP signal is recieved */
void save_auto(int signal) __attribute__((noreturn));

#endif /* _ROGUE14_SAVE_H_ */
