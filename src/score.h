#pragma once

#include "io.h"

#define SCORE_MAX 10 /* Number of highscore entries */

/* Open up the score file for future use */
int score_open(void);

/* Figure score and post it.  */
void score_show_and_exit(int death_type) __attribute__ ((noreturn));

void score_win_and_exit(void) __attribute__ ((noreturn));
