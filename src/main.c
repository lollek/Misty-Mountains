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
#include <curses.h>
#include <getopt.h>

#include "rogue.h"

static bool new_game();
static char *parse_args(int argc, char **argv);
static void endit(int sig);
static void fatal();

/** main:
 * The main program, of course
 */
int
main(int argc, char **argv)
{
  char *saved_game = NULL;

  /* get home and options from environment */
  strncpy(home, md_gethomedir(), MAXSTR);

  strcpy(file_name, home);
  strcat(file_name, "rogue.save");

  parse_opts(getenv("ROGUEOPTS"));
  if (whoami[0] == '\0')
    strucpy(whoami, md_getusername(), (int) strlen(md_getusername()));
  seed = dnum = time(NULL) + getpid();

  open_score();

  /* Drop setuid/setgid after opening the scoreboard file.  */
  md_normaluser();

  /* Play game! */
  saved_game = parse_args(argc, argv);
  return saved_game == NULL ? new_game() : restore(saved_game);
}

/** endit:
 * Exit the program abnormally.
 */

void
endit(int sig)
{
  NOOP(sig);
  fatal("Okay, bye bye!\n");
}

/** fatal:
 * Exit the program, printing a message.
 */
void
fatal(char *s)
{
  mvaddstr(LINES - 2, 0, s);
  refresh();
  endwin();
  exit(0);
}

/** playit:
 * The main loop of the program.  Loop until the game is over,
 * refreshing things and looking at the proper times.
 */

bool
playit()
{

  /* Try to crash cleanly, and autosave if possible */
  signal(SIGHUP, auto_save);
  signal(SIGQUIT, endit);
  signal(SIGILL, auto_save);
  signal(SIGTRAP, auto_save);
  signal(SIGIOT, auto_save);
  signal(SIGFPE, auto_save);
  signal(SIGBUS, auto_save);
  signal(SIGSEGV, auto_save);
  signal(SIGSYS, auto_save);
  signal(SIGTERM, auto_save);
  signal(SIGINT, quit);

  /* set up defaults for slow terminals */
  if (baudrate() <= 1200)
  {
    terse = TRUE;
    jump = TRUE;
    see_floor = FALSE;
  }

  if (md_hasclreol())
    inv_type = INV_CLEAR;

  /* parse environment declaration of options 
  parse_opts(getenv("ROGUEOPTS")); */


  oldpos = hero;
  oldrp = roomin(&hero);
  while (playing)
    command();   /* Command execution */
  endit(0);
  return 0;
}

/** quit:
 * Have player make certain, then exit.
 */
void
quit(int sig)
{
  int oy, ox;

  NOOP(sig);

  /* Reset the signal in case we got here via an interrupt */
  if (!q_comm)
    mpos = 0;
  getyx(curscr, oy, ox);
  msg("really quit?");
  if (getch() == 'y')
  {
    signal(SIGINT, leave);
    clear();
    mvprintw(LINES - 2, 0, "You quit with %d gold pieces", purse);
    move(LINES - 1, 0);
    refresh();
    score(purse, 1, 0);
    exit(0);
  }
  else
  {
    move(0, 0);
    clrtoeol();
    status();
    move(oy, ox);
    refresh();
    mpos = 0;
    count = 0;
    to_death = FALSE;
  }
}

/** leave:
 * Leave quickly, but curteously
 */
void
leave(int sig)
{
  static char buf[BUFSIZ];

  NOOP(sig);

  setbuf(stdout, buf);	/* throw away pending output */

  if (!isendwin())
  {
    mvcur(0, COLS - 1, LINES - 1, 0);
    endwin();
  }

  putchar('\n');
  exit(0);
}

/** shell:
 * Let them escape for a while
 */
void
shell()
{
  /* Set the terminal back to original mode */
  move(LINES-1, 0);
  refresh();
  endwin();
  putchar('\n');
  in_shell = TRUE;
  after = FALSE;
  fflush(stdout);

  /* Return to shell */
  kill(getpid(), SIGSTOP);

  /* Set the terminal to gaming mode */
  fflush(stdout);
  noecho();
  raw();
  in_shell = FALSE;
  clearok(stdscr, TRUE);
}

bool
new_game()
{
  if (wizard)
    printf("Hello %s, welcome to dungeon #%d", whoami, dnum);
  else
    printf("Hello %s, just a moment while I dig the dungeon...", whoami);
  fflush(stdout);

  /* Init Graphics */
  if (init_graphics() != 0)
    return FALSE;
  idlok(stdscr, TRUE);
  idlok(hw, TRUE);

  /* Init stuff */
  init_probs();                         /* Set up prob tables for objects */
  init_player();                        /* Set up initial player stats */
  init_names();                         /* Set up names of scrolls */
  init_colors();                        /* Set up colors of potions */
  init_stones();                        /* Set up stone settings of rings */
  init_materials();                     /* Set up materials of wands */

  new_level();                          /* Draw current level */

  /* Start up daemons and fuses */
  start_daemon(runners, 0, AFTER);
  start_daemon(doctor, 0, AFTER);
  fuse(swander, 0, WANDERTIME, AFTER);
  start_daemon(stomach, 0, AFTER);

  playit();

  return TRUE;
}

/** parse_args
 * Parse command-line arguments
 */
char *
parse_args(int argc, char **argv)
{
  const char *version_string = "Rogue14 Mod 1 - Based on Rogue5.4.4";
  char *saved_game = NULL;
  int option_index = 0;
  struct option long_options[] = {
    {"escdelay",  optional_argument, 0, 'E'},
    {"flush",     no_argument,       0, 'f'},
    {"hide-floor",no_argument,       0, 'F'},
    {"inv-type",  required_argument, 0, 'i'},
    {"jump",      no_argument,       0, 'j'},
    {"name",      required_argument, 0, 'n'},
    {"passgo",    no_argument,       0, 'p'},
    {"restore",   no_argument,       0, 'r'},
    {"score",     no_argument,       0, 's'},
    {"seed",      required_argument, 0, 'S'},
    {"terse",     no_argument,       0, 't'},
    {"hide-tomb", no_argument,       0, 'T'},
    {"wizard",    no_argument,       0, 'W'},
    {"help",      no_argument,       0, '0'},
    {"version",   no_argument,       0, '1'},
    {0,           0,                 0,  0 }
  };

  /* Global options */
  ESCDELAY = 0;                 /* Set the delay before ESC cancels */
  terse = FALSE;                /* Terse output */
  fight_flush = FALSE;          /* Flush typeahead during battle */
  jump = FALSE;                 /* Show running as a series of jumps */
  see_floor = TRUE;             /* Show the lamp-illuminated floor */
  passgo = FALSE;               /* Follow the turnings in passageways */
  tombstone = TRUE;             /* Print out tombstone when killed */
  inv_type = 0;                 /* Inventory style */

  for (;;)
  {
    int c = getopt_long(argc, argv, "E::fFi:jn:prsS:tTW",
                        long_options, &option_index);
    if (c == -1)
      break;

    switch (c)
    {
      case 'E': ESCDELAY = optarg == NULL ? 64 : atoi(optarg); break;
      case 'f': fight_flush = TRUE; break;
      case 'F': see_floor = FALSE; break;
      case 'i': if (atoi(optarg) >= 0 && atoi(optarg) <= 2)
                  inv_type = atoi(optarg);
                break;
      case 'j': jump = TRUE; break;
      case 'n': if (strlen(optarg))
                  strucpy(whoami, optarg, strlen(optarg));
                break;
      case 'p': passgo = TRUE; break;
      case 'r': saved_game = "-r"; break;
      case 's': noscore = TRUE; score(0, -1, 0); exit(0);
      case 'S': seed = dnum = atoi(optarg); break;
      case 't': terse = TRUE; break;
      case 'T': tombstone = FALSE; break;
      case 'W': potential_wizard = wizard = noscore = TRUE;
                player.t_flags |= SEEMONST; break;
      case '0':
        printf("Usage: %s [OPTIONS] [FILE]\n"
               "Run Rogue14 with selected options or a savefile\n\n"
               "  -E, --escdelay=[NUM] set escdelay in ms. Not settings this\n"
               "                       defaults to 0. If you do not give a NUM\n"
               "                       argument, it's set to 64 (old standard)\n"
               "  -f, --flush          flush typeahead during battle\n"
               "  -F, --hide-floor     hide the lamp-illuminated floor\n"
               "  -i, --inv-type=0-2   change inventory style\n"
               "                       0 - Inventory over\n"
               "                       1 - Inventory slow\n"
               , argv[0]); printf(
               "                       2 - Inventory clear\n"
               "  -j, --jump           show running as a series of jumps\n"
               "  -n, --name=NAME      set highscore name\n"
               "  -p, --passgo         Follow the turnings in passageways\n"
               "  -r, --restore        restore game to default\n"
               "  -s, --score          display the highscore and exit\n"
               "  -S, --seed=NUMBER    set map seed to NUMBER\n"
               "  -t, --terse          terse output\n"
               "  -T, --hide-tomb      don't print out tombstone when killed\n"
               "  -W, --wizard         run the game in debug-mode\n"
               ); printf(
               "      --help           display this help and exit\n"
               "      --version        display game version and exit\n\n"
               "%s\n"
               , version_string);
        exit(0);
      case '1': puts(version_string); exit(0);
      default:
        fprintf(stderr, "Try '%s --help' for more information\n",
                argv[0]);
        exit(1);
    }
  }

  if (optind < argc)
    saved_game = argv[optind];

  return saved_game;
}

