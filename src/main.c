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

void parse_args(int argc, char **argv);

/** main:
 * The main program, of course
 */
int
main(int argc, char **argv)
{
  char *env;

  /* from md_init - try to remove these */
  ESCDELAY = 0; /* Set the delay before ESC cancels */
  signal(SIGHUP, SIG_DFL);
  signal(SIGQUIT, exit);
  signal(SIGILL, exit);
  signal(SIGTRAP, exit);
  signal(SIGIOT, exit);
  signal(SIGFPE, exit);
  signal(SIGBUS, exit);
  signal(SIGSEGV, exit);
  signal(SIGSYS, exit);
  signal(SIGTERM, exit);

#ifdef MASTER
  /* Check to see if he is a wizard */
  if (argc >= 2 && argv[1][0] == '\0' &&
      strcmp(PASSWD, md_crypt(getpass("wizard's password: "), "mT")) == 0)
  {
    wizard = TRUE;
    player.t_flags |= SEEMONST;
    argv++;
    argc--;
  }
#endif

  /* get home and options from environment */
  strncpy(home, md_gethomedir(), MAXSTR);

  strcpy(file_name, home);
  strcat(file_name, "rogue.save");

  if ((env = getenv("ROGUEOPTS")) != NULL)
    parse_opts(env);
  if (env == NULL || whoami[0] == '\0')
    strucpy(whoami, md_getusername(), (int) strlen(md_getusername()));
  dnum = time(NULL) + getpid();
  seed = dnum;

  open_score();

  /* Drop setuid/setgid after opening the scoreboard file.  */
  md_normaluser();

  init_check();   /* check for legal startup */
  parse_args(argc, argv);

#ifdef MASTER
  if (wizard)
    printf("Hello %s, welcome to dungeon #%d", whoami, dnum);
  else
#endif
    printf("Hello %s, just a moment while I dig the dungeon...", whoami);
  fflush(stdout);

  initscr();				/* Start up cursor package */
  init_probs();			/* Set up prob tables for objects */
  init_player();			/* Set up initial player stats */
  init_names();			/* Set up names of scrolls */
  init_colors();			/* Set up colors of potions */
  init_stones();			/* Set up stone settings of rings */
  init_materials();			/* Set up materials of wands */
  setup();

  /* The screen must be at least NUMLINES x NUMCOLS */
  if (LINES < NUMLINES || COLS < NUMCOLS)
  {
    printf("\nSorry, the screen must be at least %dx%d\n", NUMLINES, NUMCOLS);
    endwin();
    exit(1);
  }

  /* Set up windows */
  hw = newwin(LINES, COLS, 0, 0);
  idlok(stdscr, TRUE);
  idlok(hw, TRUE);
#ifdef MASTER
  noscore = wizard;
#endif
  new_level();  /* Draw current level */

  /* Start up daemons and fuses */
  start_daemon(runners, 0, AFTER);
  start_daemon(doctor, 0, AFTER);
  fuse(swander, 0, WANDERTIME, AFTER);
  start_daemon(stomach, 0, AFTER);
  playit();
  return(0);
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

/** rnd:
 * Pick a very random number.
 */
int
rnd(int range)
{
  return range == 0 ? 0 : abs((int) RN) % range;
}

/** roll:
 * Roll a number of dice
 */
int
roll(int number, int sides)
{
  int dtotal = 0;

  while (number--)
    dtotal += rnd(sides)+1;
  return dtotal;
}

/** tstp:
 * Handle stop and start signals
 */
void
tstp(int ignored)
{
  int y, x;
  int oy, ox;

  NOOP(ignored);

  /* leave nicely */
  getyx(curscr, oy, ox);
  mvcur(0, COLS - 1, LINES - 1, 0);
  endwin();
  fflush(stdout);

  /* start back up again */
  raw();
  noecho();
  keypad(stdscr,1);
  clearok(curscr, TRUE);
  wrefresh(curscr);
  getyx(curscr, y, x);
  mvcur(y, x, oy, ox);
  fflush(stdout);
  curscr->_cury = oy;
  curscr->_curx = ox;
}

/** playit:
 * The main loop of the program.  Loop until the game is over,
 * refreshing things and looking at the proper times.
 */

void
playit()
{
  char *opts;

  /* set up defaults for slow terminals */
  if (baudrate() <= 1200)
  {
    terse = TRUE;
    jump = TRUE;
    see_floor = FALSE;
  }

  if (md_hasclreol())
    inv_type = INV_CLEAR;

  /* parse environment declaration of options */
  if ((opts = getenv("ROGUEOPTS")) != NULL)
    parse_opts(opts);


  oldpos = hero;
  oldrp = roomin(&hero);
  while (playing)
    command();   /* Command execution */
  endit(0);
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
  if (readchar() == 'y')
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

/* shell:
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

  fflush(stdout);
  noecho();
  raw();
  keypad(stdscr,1);
  in_shell = FALSE;
  clearok(stdscr, TRUE);
}

void
parse_args(int argc, char **argv)
{
  const char *version_string = "Rogue14 mod1 - Based on Rogue5.4.4";
  int option_index = 0;
  struct option long_options[] = {
    {"restore",   no_argument,       0, 'r'},
    {"score",     no_argument,       0, 's'},
    {"seed",      required_argument, 0, 'S'},
    {"help",      no_argument,       0, '0'},
    {"version",   no_argument,       0, '1'},
    {0,           0,                 0,  0 }
  };

  for (;;)
  {
    int c = getopt_long(argc, argv, "rsS:", long_options, &option_index);
    if (c == -1)
      break;

    switch (c)
    {
      case 'r':
        if (!restore("-r"))  /* Note: restore will never return */
          exit(1);
      case 's':
        noscore = TRUE;
        score(0, -1, 0);
        exit(0);
      case 'S':
        seed = dnum = atoi(optarg);
        break;
      case '0':
        printf("Usage: %s [OPTIONS] [FILE]\n"
               "Run Rogue14 with selected options or a savefile\n\n"
               "  -r, --restore        restore game to default\n"
               "  -s, --score          display the highscore and exit\n"
               "  -S, --seed=NUMBER    set map seed to NUMBER\n"
               "      --help           display this help and exit\n"
               "      --version        display game version and exit\n\n"
               "%s\n", argv[0], version_string);
        exit(0);
      case '1':
        puts(version_string);
        exit(0);
      default:
        fprintf(stderr, "Try '%s --help' for more information\n",
                argv[0]);
        exit(1);
    }
  }

  if (optind < argc)
  {
    if (!restore(argv[optind]))  /* Note: restore will never return */
      exit(1);
  }
}

