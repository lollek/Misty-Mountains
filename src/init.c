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

#include "potions.h"
#include "scrolls.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "daemons.h"
#include "colors.h"
#include "list.h"
#include "level.h"
#include "rings.h"
#include "save.h"
#include "misc.h"
#include "player.h"
#include "weapons.h"
#include "wand.h"
#include "state.h"
#include "things.h"
#include "options.h"
#include "rogue.h"

#include "init.h"

/** sumprobs:
 * Sum up the probabilities for items appearing */
static void
sumprobs(char ch)
{
  char const* str;
  void const* ptr;
  int max;

  /* Ready the pointers */
  switch (ch)
  {
    case '0':    ptr = things;         max = NUMTHINGS;  str = "things"; break;
    case POTION: ptr = potion_info;    max = NPOTIONS;   str = "potions"; break;
    case SCROLL: ptr = scroll_info;    max = NSCROLLS;   str = "scrolls"; break;
    case RING:   ptr = ring_info;      max = NRINGS;     str = "rings"; break;
    case STICK:  ptr = __wands_ptr();  max = MAXSTICKS;  str = "wands"; break;
    case WEAPON: ptr = weapon_info;    max = MAXWEAPONS; str = "weapons"; break;
    case ARMOR:  ptr = NULL;           max = NARMORS;    str = "armor"; break;
    default:     ptr = NULL;           max = 0;          str = "error"; break;
  }

  /* Add upp percentage */
  int sum = 0;
  for (int i = 0; i < max; ++i)
  {
    if (ch == ARMOR) sum += armor_probability((enum armor_t)i);
    else             sum += ((struct obj_info*)ptr)[i].oi_prob;
  }

  /* Make sure it adds up to 100 */
  if (sum == 100)
    return;

  /* Woops, error error! */
  endwin();
  printf("\nBad percentages for %s (bound = %d): %d%%\n", str, max, sum);
  for (int i = 0; i < max; ++i)
  {
    int prob;
    char const* name;
    if (ch == ARMOR)
    {
      prob = armor_probability((enum armor_t)i);
      name = armor_name((enum armor_t)i);
    }
    else
    {
      prob = ((struct obj_info*)ptr)[i].oi_prob;
      name = ((struct obj_info*)ptr)[i].oi_name;
    }
    printf("%3d%% %s\n", prob, name);
  }
  exit(1);
}

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

/** init_probs:
 * Initialize the probabilities for the various items */
static void
init_probs(void)
{
  sumprobs('0');
  sumprobs(POTION);
  sumprobs(SCROLL);
  sumprobs(RING);
  sumprobs(STICK);
  sumprobs(WEAPON);
  sumprobs(ARMOR);
}

bool
init_new_game(void)
{
  /* Parse environment opts */
  if (whoami[0] == '\0')
  {
    struct passwd const* pw = getpwuid(getuid());
    char const* name = pw ? pw->pw_name : "nobody";

    strucpy(whoami, name, (int)strlen(name));
  }

  printf("Hello %s, just a moment while I dig the dungeon...", whoami);
  fflush(stdout);

  /* Init Graphics */
  if (init_graphics() != 0)
    return false;
  idlok(stdscr, true);
  idlok(hw, true);

  /* Init stuff */
  init_probs();                         /* Set up prob tables for objects */
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

bool
init_old_game(void)
{
  FILE* inf = fopen(save_file_name, "r");
  char buf[MAXSTR];

  if (inf == NULL)
  {
    perror(save_file_name);
    return false;
  }

  fflush(stdout);
  io_encread(buf, sizeof(GAME_VERSION), inf);
  if (strcmp(buf, GAME_VERSION))
  {
    printf("Sorry, saved game is out of date.\n");
    return false;
  }

  io_encread(buf, 80, inf);

  if (init_graphics() != 0)
    return false;

  if (state_load_file(inf) != 0)
  {
    endwin();
    printf(": Corrupted save game\n");
    return false;
  }

  /* we do not close the file so that we will have a hold of the
   * inode for as long as possible */

  if (unlink(save_file_name) < 0)
  {
    endwin();
    printf("Cannot unlink file\n");
    return false;
  }
  mpos = 0;
  clearok(stdscr,true);

  if (player_get_health() <= 0)
  {
    endwin();
    printf("\n\"He's dead, Jim\"\n");
    return false;
  }

  clearok(curscr, true);
  msg("file name: %s", save_file_name);
  return true;
}


