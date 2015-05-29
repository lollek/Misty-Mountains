/*
 * This file has all the code for the option command.  I would rather
 * this command were not necessary, but it is the only way to keep the
 * wolves off of my back.
 *
 * @(#)options.c	4.24 (Berkeley) 05/10/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "io.h"
#include "misc.h"
#include "player.h"
#include "rogue.h"

#include "options.h"

bool fight_flush  = false;
bool jump         = true;
bool passgo       = false;
bool use_colors   = true;

static bool
get_bool(void* vp, WINDOW* win)
{
  wrefresh(win);
  bool* b = vp;
  *b = !*b;
  waddstr(win, *b ? "True " : "False");
  return 0;
}

static inline bool
get_str(void* vopt, WINDOW* win)
{
  return wreadstr(win, (char*)vopt);
}

/** option:
 * Print and then set options from the terminal */
bool
option(void)
{
  struct option {
    char const* o_prompt; /* prompt for interactive entry */
    void* o_opt;          /* pointer to thing to set function to print value */
    enum put_t { PUT_BOOL, PUT_STR } put_type;
    bool (*o_getfunc)(void *opt, WINDOW *win); /* Get value */
  } optlist[] = {
    {"Flush typeahead during battle",    &fight_flush, PUT_BOOL, get_bool},
    {"Show position only at end of run", &jump,        PUT_BOOL, get_bool},
    {"Follow turnings in passageways",   &passgo,      PUT_BOOL, get_bool},
    {"Name",                             whoami,       PUT_STR,  get_str},
    {"Save file",                        file_name,    PUT_STR,  get_str}
  };
  int const NOPTS = (sizeof optlist / sizeof (*optlist));
  char const* query = "Which value do you want to change? (ESC to exit) ";
  coord const msg_pos =
  {
    .y = 0,
    .x = (int)strlen(query)
  };
  msg(query);

  WINDOW *optscr = NULL;
  optscr = dupwin(stdscr);

  /* Display current values of options */
  int i;
  wmove(optscr, 1, 0);
  for (i = 0; i < NOPTS; ++i)
  {
    wprintw(optscr, "%d: %s: ", i + 1, optlist[i].o_prompt);
    if (optlist[i].put_type == PUT_BOOL)
      waddstr(optscr, *(bool *) optlist[i].o_opt ? "True" : "False");
    else /* PUT_STR */
      waddstr(optscr, (char *) optlist[i].o_opt);
    waddch(optscr, '\n');
  }

  char c = (char)~KEY_ESCAPE;
  do
  {
    wmove(optscr, msg_pos.y, msg_pos.x);
    wrefresh(optscr);
    c = readchar(true);
    if (c > '0' && c <= '0' + NOPTS)
    {
      i = c - '0' - 1;
      mvwprintw(optscr, i + 1, 0, "%d: %s: ", i + 1, optlist[i].o_prompt);
      (*optlist[i].o_getfunc)(optlist[i].o_opt, optscr);
    }
  }
  while (c != KEY_ESCAPE);

  /* Switch back to original screen */
  wmove(optscr, LINES - 1, 0);
  delwin(optscr);
  clearok(curscr, true);
  touchwin(stdscr);
  clearmsg();
  return false;
}

