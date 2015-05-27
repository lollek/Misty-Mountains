#ifndef _ROGUE14_SAVE_H_
#define _ROGUE14_SAVE_H_

#include <stdbool.h>
#include <stdio.h>

bool save_game(void);

/* Automatically save a file.  This is used if a HUP signal is recieved */
void auto_save(int signal) __attribute__((noreturn));

/* Perform an encrypted write */
size_t encwrite(const char *start, size_t size, FILE *outf);

/* Perform an encrypted read */
size_t encread(char *start, size_t size, FILE *inf);

#endif /* _ROGUE14_SAVE_H_ */
