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
#include <time.h>
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

#include "rip.h"


/** center:
 * Return the index to center the given string */
static int
center(char const* str)
{
  return 28 - (((int)strlen(str) + 1) / 2);
}


/** killname:
 * Convert a code to a monster name */
static char *
killname(char buf[], char monst, bool doart)
{
  struct list
  {
    char ch;
    char const* description;
    bool print;
  } nlist[] = {
    {'a',	"arrow",		true},
    {'b',	"bolt",			true},
    {'d',	"dart",			true},
    {'f',	"flame",		false},
    {'h',	"hypothermia",		false},
    {'i',	"hypothermia",		false},
    {'s',	"starvation",		false},
  };

  char const* sp;
  bool article = true;
  if (isupper(monst))
    sp = monster_name_by_type(monst);

  else
  {
    sp = "Wally the Wonder Badger";
    article = false;
    for (unsigned i = 0; i < sizeof(nlist) / sizeof(*nlist); ++i)
      if (nlist[i].ch == monst)
      {
        sp = nlist[i].description;
        article = nlist[i].print;
        break;
      }
  }

  if (doart && article)
    sprintf(buf, "a%s ", vowelstr(sp));
  else
    buf[0] = '\0';
  strcat(buf, sp);
  return buf;
}

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
  char* reason[] = { "killed", "quit", "A total winner", "killed with Amulet" };
  char buf[2*MAXSTR];
  if (flags != -1)
    putchar('\n');

  printf("Top %s %s:\n", SCORE_MAX_STRING, "Scores");
  printf("   Score Name\n");
  for (SCORE* ptr = top_ten; ptr < endp; ptr++)
  {
    if (!ptr->sc_score)
      break;

    printf("%2d %5d %s: %s on level %d"
        ,(int)(ptr - top_ten + 1)/* Position */
        ,ptr->sc_score           /* Score */
        ,ptr->sc_name            /* Name */
        ,reason[ptr->sc_flags]   /* Cause of death */
        ,ptr->sc_level);         /* Death level */

    if (ptr->sc_flags == 0 || ptr->sc_flags == 3)
      printf(" by %s", killname(buf, (char) ptr->sc_monster, true));

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
death(char monst)
{
  char buf[2*MAXSTR];

  signal(SIGINT, SIG_IGN);

  pack_gold -= pack_gold / 10;

  signal(SIGINT, command_signal_leave);

  status();
  refresh();
  msg("You die!");
  readchar(false);

  clear();
  char const* killer = killname(buf, monst, false);

  time_t date;
  time(&date);
  struct tm* lt = localtime(&date);

  move(8, 0);

  char const* rip[] = {
    "                       __________\n",
    "                      /          \\\n",
    "                     /    REST    \\\n",
    "                    /      IN      \\\n",
    "                   /     PEACE      \\\n",
    "                  /                  \\\n",
    "                  |                  |\n",
    "                  |                  |\n",
    "                  |   killed by a    |\n",
    "                  |                  |\n",
    "                  |       1980       |\n",
    "                 *|     *  *  *      | *\n",
    "         ________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______\n",
    0
  };
  char const** dp = rip;
  while (*dp)
    addstr(*dp++);

  mvaddstr(17, center(killer), killer);
  if (monst == 's' || monst == 'h')
    mvaddcch(16, 32, ' ');
  else
    mvaddstr(16, 33, vowelstr(killer));

  mvaddstr(14, center(whoami), whoami);
  sprintf(buf, "%d Au", pack_gold);
  move(15, center(buf));
  addstr(buf);
  sprintf(buf, "%4d", 1900+lt->tm_year);
  mvaddstr(18, 26, buf);

  mvprintw(LINES -1, 0, "[Press return to continue]");
  refresh();
  wait_for(KEY_ENTER);
  pack_evaluate();
  score(pack_gold, pack_contains_amulet() ? 3 : 0, monst);
  exit(0);
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


