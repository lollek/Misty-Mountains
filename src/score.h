#ifndef ROGUE14_SCORE_H
#define ROGUE14_SCORE_H

#include "io.h"

#define SCORE_MAX 10 /* Number of highscore entries */

/* Open up the score file for future use
 * We drop setgid privileges after opening the score file, so subsequent
 * open()'s will fail.  Just reuse the earlier filehandle. */
int score_open_and_drop_setuid_setgid(void);

/* Figure score and post it.  */
void score_show_and_exit(int amount, int flags, char monst)
  __attribute__ ((noreturn));

void score_win_and_exit(void) __attribute__ ((noreturn));

#endif /* ROGUE14_SCORE_H */
