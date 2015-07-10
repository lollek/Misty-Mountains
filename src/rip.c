/*
 * File for the fun ends
 * Death or a total win
 *
 * @(#)rip.c	4.57 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>

#include "armor.h"
#include "command.h"
#include "io.h"
#include "level.h"
#include "misc.h"
#include "monster.h"
#include "os.h"
#include "pack.h"
#include "potions.h"
#include "rogue.h"
#include "score.h"
#include "scrolls.h"
#include "wizard.h"
#include "death.h"

#include "rip.h"

static bool
score_insert(SCORE* top_ten, SCORE* endp, int amount, int flags, char monst)
{
  unsigned uid = getuid();
  SCORE* ptr;
  for (ptr = top_ten; ptr < endp; ptr++)
    if (amount > ptr->sc_score)
      break;

  if (ptr == endp)
    return false;

  SCORE* sc2 = endp - 1;
  while (sc2 > ptr)
  {
    *sc2 = sc2[-1];
    sc2--;
  }

  ptr->sc_score = amount;
  strncpy(ptr->sc_name, whoami, MAXSTR);
  ptr->sc_flags = flags;
  ptr->sc_level = flags == 2
    ? level_max
    : level;
  ptr->sc_monster = monst;
  ptr->sc_uid = uid;
  sc2 = ptr;

  return true;
}

static void
score_print(SCORE* top_ten, SCORE* endp, int flags, int prflags)
{
  char buf[2*MAXSTR];
  if (flags != -1)
    putchar('\n');

  printf("Top %s %s:\n", SCORE_MAX_STRING, "Scores");
  printf("   Score Name\n");
  for (SCORE* ptr = top_ten; ptr < endp; ptr++)
  {
    if (!ptr->sc_score)
      break;

    printf("%2d %5d %s: "
        ,(int)(ptr - top_ten + 1)/* Position */
        ,ptr->sc_score           /* Score */
        ,ptr->sc_name            /* Name */
        );

    if (ptr->sc_flags == 0)
      printf("%s", death_reason(buf, ptr->sc_monster));
    else if (ptr->sc_flags == 1)
      printf("Quit");
    else if (ptr->sc_flags == 2)
      printf("A total winner");
    else if (ptr->sc_flags == 3)
      printf("%s while holding the amulet", death_reason(buf, ptr->sc_monster));

    printf(" on level %d", ptr->sc_level);

    if (prflags == 2)
    {
      fflush(stdout);
      (void) fgets(buf,10,stdin);
      if (buf[0] == 'd')
      {
        SCORE* sc2;
        for (sc2 = ptr; sc2 < endp - 1; sc2++)
          *sc2 = *(sc2 + 1);
        sc2 = endp - 1;
        sc2->sc_score = 0;
        for (int i = 0; i < MAXSTR; i++)
          sc2->sc_name[i] = (char) os_rand_range(255);
        sc2->sc_flags = os_rand();
        sc2->sc_level = os_rand();
        sc2->sc_monster = os_rand();
        ptr--;
      }
    }
    else
      printf(".");
    putchar('\n');
  }
}

void
score(int amount, int flags, char monst)
{
  int prflags = 0;
  char buf[2*MAXSTR];

  if (flags >= 0 || wizard)
  {
    mvaddstr(LINES - 1, 0 , "[Press return to continue]");
    refresh();
    wgetnstr(stdscr,buf,80);
    endwin();
    printf("\n");

    /* free up space to "guarantee" there is space for the top_ten */
    delwin(stdscr);
    delwin(curscr);
    if (hw != NULL)
      delwin(hw);
  }

  SCORE* top_ten = malloc(SCORE_MAX * sizeof (SCORE));
  SCORE* endp = &top_ten[SCORE_MAX];
  for (SCORE* ptr = top_ten; ptr < endp; ptr++)
  {
    ptr->sc_score = 0;
    for (int i = 0; i < MAXSTR; i++)
      ptr->sc_name[i] = (unsigned char) os_rand_range(255);
    ptr->sc_flags = os_rand();
    ptr->sc_level = os_rand();
    ptr->sc_monster = os_rand();
    ptr->sc_uid = (unsigned)os_rand();
  }

  signal(SIGINT, SIG_DFL);

  if (wizard && !strcmp(buf, "edit"))
    prflags = 2;

  score_read(top_ten);

  /* Insert her in list if need be */
  bool in_highscore = false;
  if (!wizard)
    in_highscore = score_insert(top_ten, endp, amount, flags, monst);

  /* Print the highscore */
  score_print(top_ten, endp, flags, prflags);

  /** Update the list file */
  if (in_highscore)
    score_write(top_ten);

  free(top_ten);
  top_ten = NULL;
}


void
total_winner(void)
{
  clear();
  addstr("                                                               \n");
  addstr("  @   @               @   @           @          @@@  @     @  \n");
  addstr("  @   @               @@ @@           @           @   @     @  \n");
  addstr("  @   @  @@@  @   @   @ @ @  @@@   @@@@  @@@      @  @@@    @  \n");
  addstr("   @@@@ @   @ @   @   @   @     @ @   @ @   @     @   @     @  \n");
  addstr("      @ @   @ @   @   @   @  @@@@ @   @ @@@@@     @   @     @  \n");
  addstr("  @   @ @   @ @  @@   @   @ @   @ @   @ @         @   @  @     \n");
  addstr("   @@@   @@@   @@ @   @   @  @@@@  @@@@  @@@     @@@   @@   @  \n");
  addstr("                                                               \n");
  addstr("     Congratulations, you have made it to the light of day!    \n");
  addstr("\nYou have joined the elite ranks of those who have escaped the\n");
  addstr("Dungeons of Doom alive.  You journey home and sell all your loot at\n");
  addstr("a great profit and are admitted to the Fighters' Guild.\n");
  mvaddstr(LINES - 1, 0, "--Press space to continue--");
  refresh();
  wait_for(KEY_SPACE);
  pack_gold += pack_evaluate();
  score(pack_gold, 2, ' ');
  exit(0);
}


