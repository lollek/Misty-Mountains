/*
 * Score file structure
 *
 * @(#)score.h	4.6 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

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
