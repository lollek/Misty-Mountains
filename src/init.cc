/*
 * global variable initializaton
 *
 * @(#)init.c	4.31 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string>

using namespace std;

#include "potions.h"
#include "scrolls.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "daemons.h"
#include "colors.h"
#include "level.h"
#include "rings.h"
#include "misc.h"
#include "player.h"
#include "weapons.h"
#include "wand.h"
#include "things.h"
#include "options.h"
#include "rogue.h"

#include "init.h"

/** init_graphics:
 *  get curses running */
static int
init_graphics(void)
{
  initscr();  /* Start up cursor package */

  /* Ncurses colors */
  if (use_colors)
  {
    if (start_color() == ERR)
    {
      endwin();
      fprintf(stderr, "Error: Failed to start colors. "
                      "Try restarting without colors enabled\n");
      return 1;
    }

    /* Because ncurses has defined COLOR_BLACK to 0 and COLOR_WHITE to 7,
     * and then decided that init_pair cannot change number 0 (COLOR_BLACK)
     * I use COLOR_WHITE for black text and COLOR_BLACK for white text */

    assume_default_colors(0, -1); /* Default is white text and any background */
    init_pair(COLOR_RED, COLOR_RED, -1);
    init_pair(COLOR_GREEN, COLOR_GREEN, -1);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, -1);
    init_pair(COLOR_BLUE, COLOR_BLUE, -1);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, -1);
    init_pair(COLOR_CYAN, COLOR_CYAN, -1);
    init_pair(COLOR_WHITE, COLOR_BLACK, -1);
  }

  if (LINES < NUMLINES || COLS < NUMCOLS)
  {
    endwin();
    printf("\nSorry, the screen must be at least %dx%d\n", NUMLINES, NUMCOLS);
    return 1;
  }

  raw();     /* Raw mode */
  noecho();  /* Echo off */
  hw = newwin(LINES, COLS, 0, 0);

  return 0;
}

bool
init_new_game(void)
{
  /* Parse environment opts */
  if (whoami[0] == '\0')
  {
    struct passwd const* pw = getpwuid(getuid());
    char const* name = pw ? pw->pw_name : "nobody";

    strucpy(whoami, name, static_cast<int>(strlen(name)));
  }

  printf("Hello %s, just a moment while I dig the dungeon...", whoami);
  fflush(stdout);

  /* Init Graphics */
  if (init_graphics() != 0)
    return false;
  idlok(stdscr, true);
  idlok(hw, true);

  /* Init stuff */
  player_init();                        /* Set up initial player stats */
  scroll_init();                        /* Set up names of scrolls */
  potions_init();                       /* Set up colors of potions */
  ring_init();                          /* Set up stone settings of rings */
  wand_init();                          /* Set up materials of wands */

  level_new();                          /* Draw current level */

  /* Start up daemons and fuses */
  daemon_start(daemon_runners_move, 0, AFTER);
  daemon_start(daemon_doctor, 0, AFTER);
  daemon_start(daemon_ring_abilities, 0, AFTER);

  return true;
}

