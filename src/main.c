/*
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 *
 * @(#)main.c 4.22 (Berkeley) 02/05/99
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>

#include "command.h"
#include "io.h"
#include "pack.h"
#include "init.h"
#include "save.h"
#include "score.h"
#include "misc.h"
#include "rooms.h"
#include "player.h"
#include "rip.h"
#include "rogue.h"

enum game_mode_t
{
  LOAD_GAME = 0,
  NEW_GAME = 1
};

/** parse_args
 * Parse command-line arguments
 */
static enum game_mode_t
parse_args(int argc, char * const *argv)
{
  enum game_mode_t game_mode = NEW_GAME;
  int option_index = 0;
  struct option long_options[] = {
    {"no-colors", no_argument,       0, 'c'},
    {"escdelay",  optional_argument, 0, 'E'},
    {"flush",     no_argument,       0, 'f'},
    {"hide-floor",no_argument,       0, 'F'},
    {"jump",      no_argument,       0, 'j'},
    {"name",      required_argument, 0, 'n'},
    {"passgo",    no_argument,       0, 'p'},
    {"restore",   no_argument,       0, 'r'},
    {"score",     no_argument,       0, 's'},
    {"seed",      required_argument, 0, 'S'},
    {"terse",     no_argument,       0, 't'},
    {"hide-tomb", no_argument,       0, 'T'},
    {"quick",     no_argument,       0, 'Q'},
    {"wizard",    no_argument,       0, 'W'},
    {"help",      no_argument,       0, '0'},
    {"version",   no_argument,       0, '1'},
    {0,           0,                 0,  0 }
  };

  /* Global default options */
  ESCDELAY = 0;                 /* Set the delay before ESC cancels */
  terse = false;                /* Terse output */
  fight_flush = false;          /* Flush typeahead during battle */
  jump = false;                 /* Show running as a series of jumps */
  see_floor = true;             /* Show the lamp-illuminated floor */
  passgo = false;               /* Follow the turnings in passageways */
  tombstone = true;             /* Print out tombstone when killed */
  use_colors = true;            /* Use ncurses colors */
  game_type = DEFAULT;          /* Play a normal game or rogue */

  /* Default file name for save file */
  strcpy(file_name, get_homedir());
  strncat(file_name, ".rogue14_save", MAXSTR - strlen(get_homedir()) -1);

  /* Set seed and dungeon number */
  seed = time(NULL) + getpid();

  for (;;)
  {
    int c = getopt_long(argc, argv, "cE::fFjn:prsS:tTQW",
                        long_options, &option_index);
    if (c == -1)
      break;

    switch (c)
    {
      case 'c': use_colors = false; break;
      case 'E': ESCDELAY = optarg == NULL ? 64 : atoi(optarg); break;
      case 'f': fight_flush = true; break;
      case 'F': see_floor = false; break;
      case 'j': jump = true; break;
      case 'n': if (strlen(optarg))
                  strucpy(whoami, optarg, strlen(optarg));
                break;
      case 'p': passgo = true; break;
      case 'r': game_mode = LOAD_GAME; break;
      case 's': potential_wizard = true;
                score(0, -1, 0);
                exit(0);
      case 'S': seed = atoi(optarg); break;
      case 't': terse = true; break;
      case 'T': tombstone = false; break;
      case 'Q': game_type = QUICK; break;
      case 'W': potential_wizard = wizard = true; break;
      case '0':
        printf("Usage: %s [OPTIONS] [FILE]\n"
               "Run Rogue14 with selected options or a savefile\n\n"
               "  -c, --no-colors      remove colors from the game\n"
               "  -E, --escdelay=[NUM] set escdelay in ms. Not setting this\n"
               "                       defaults to 0. If you do not give a NUM\n"
               "                       argument, it's set to 64 (old standard)\n"
               "  -f, --flush          flush typeahead during battle\n"
               "  -F, --hide-floor     hide the lamp-illuminated floor\n"
               "  -j, --jump           show running as a series of jumps\n"
               , argv[0]); printf(
               "  -n, --name=NAME      set highscore name\n"
               "  -p, --passgo         Follow the turnings in passageways\n"
               "  -r, --restore        restore game instead of creating a new\n"
               "  -s, --score          display the highscore and exit\n"
               "  -S, --seed=NUMBER    set map seed to NUMBER\n"
               "  -t, --terse          terse output\n"
               "  -T, --hide-tomb      don't print out tombstone when killed\n"
               "  -Q, --quick          Change some rules to make the game take\n"
               "                       less time to play\n"
               ); printf(
               "  -W, --wizard         run the game in debug-mode\n"
               "      --help           display this help and exit\n"
               "      --version        display game version and exit\n\n"
               "%s\n"
               , GAME_VERSION);
        exit(0);
      case '1':
        puts(GAME_VERSION);
        exit(0);
      default:
        fprintf(stderr, "Try '%s --help' for more information\n",
                argv[0]);
        exit(1);
    }
  }

  /* If we run ./game -r ~/saved_game we should restore it
   * otherwise, we should create a new game with ~/saved_game as file_name */
  if (optind < argc)
  {
    strncpy(file_name, argv[optind], MAXSTR);
    file_name[MAXSTR -1] = '\0';
  }

  return game_mode;
}

/** main:
 * The main program, of course */
int
main(int argc, char **argv)
{
  bool retval = false;

  /* Open scoreboard and drop setuid/getgid, so we can modify the score later */
  if (open_score_and_drop_setuid_setgid() != 0)
    return 1;

  /* Parse args and then init new (or old) game */
  switch (parse_args(argc, argv))
  {
    case LOAD_GAME: retval = init_old_game(); break;
    case NEW_GAME:  retval = init_new_game(); break;
    default:
      printf("Error: Failed while parsing arguments\n");
      return 1;
  }

  if (retval == false)
    return 1;

  /* Try to crash cleanly, and autosave if possible */
  signal(SIGHUP, auto_save);
  signal(SIGQUIT, command_signal_endit);
  signal(SIGILL, auto_save);
  signal(SIGTRAP, auto_save);
  signal(SIGIOT, auto_save);
  signal(SIGFPE, auto_save);
  signal(SIGBUS, auto_save);
  signal(SIGSEGV, auto_save);
  signal(SIGSYS, auto_save);
  signal(SIGTERM, auto_save);
  signal(SIGINT, command_signal_quit);

  oldpos = *player_get_pos();
  oldrp = roomin(player_get_pos());
  while (command() != 1)
    /* Main Loop */;
  command_signal_endit(0);
  return 0;
}

