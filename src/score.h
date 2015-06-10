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

#define SCORE_MAX         10    /* Number of highscore entries */
#define SCORE_MAX_STRING  "Ten" /* The same number in letters  */

typedef struct sc_ent {
  unsigned sc_uid;
  int sc_score;
  int sc_flags;
  int sc_monster;
  char sc_name[MAXSTR];
  int sc_level;
  unsigned sc_time;
} SCORE;

/* Open up the score file for future use
 * We drop setgid privileges after opening the score file, so subsequent
 * open()'s will fail.  Just reuse the earlier filehandle. */
int score_open_and_drop_setuid_setgid(void);

/* Read in the score file */
void score_read(SCORE* top_ten);

/* Write out the score file */
void score_write(SCORE* top_ten);
