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

#include "rogue.h"

#include "options.h"

static bool get_bool(void *vp, WINDOW *win); /* Change a boolean */
static bool get_sf(void *vp, WINDOW *win);   /* Toggle see_floor */

#define NUM_OPTS (sizeof optlist / sizeof (OPTION))

enum put_t
{
  PUT_BOOL,
  PUT_STR
};

/* description of an option and what to do with it */
typedef struct optstruct {
  char *o_prompt;     /* prompt for interactive entry */
  void *o_opt;        /* pointer to thing to set function to print value */
  enum put_t put_type;
  bool (*o_getfunc)(void *opt, WINDOW *win); /* Get value */
} OPTION;

static OPTION optlist[] = {
  {"Terse output",                     &terse,       PUT_BOOL, get_bool},
  {"Flush typeahead during battle",    &fight_flush, PUT_BOOL, get_bool},
  {"Show position only at end of run", &jump,        PUT_BOOL, get_bool},
  {"Show the lamp-illuminated floor",  &see_floor,   PUT_BOOL, get_sf},
  {"Follow turnings in passageways",   &passgo,      PUT_BOOL, get_bool},
  {"Show tombstone when killed",       &tombstone,   PUT_BOOL, get_bool},
  {"Name",                             whoami,       PUT_STR,  get_str},
  {"Save file",                        file_name,    PUT_STR,  get_str}
};


/* option:
 * Print and then set options from the terminal */
bool
option(void)
{
  char c = ~KEY_ESCAPE;
  WINDOW *optscr = NULL;
  coord msg_pos;
  unsigned i;

  msg("Which value do you want to change? (ESC to exit) ");
  getyx(stdscr, msg_pos.y, msg_pos.x);
  optscr = dupwin(stdscr);

  /* Display current values of options */
  wmove(optscr, 1, 0);
  for (i = 0; i < NUM_OPTS; ++i)
  {
    wprintw(optscr, "%d: %s: ", i + 1, optlist[i].o_prompt);
    if (optlist[i].put_type == PUT_BOOL)
      waddstr(optscr, *(bool *) optlist[i].o_opt ? "True" : "False");
    else /* PUT_STR */
      waddstr(optscr, (char *) optlist[i].o_opt);
    waddch(optscr, '\n');
  }

  while (c != KEY_ESCAPE)
  {
    wmove(optscr, msg_pos.y, msg_pos.x);
    wrefresh(optscr);
    c = readchar();
    if (c > '0' && c <= '0' + NUM_OPTS)
    {
      i = c - '0' - 1;
      mvwprintw(optscr, i + 1, 0, "%d: %s: ", i + 1, optlist[i].o_prompt);
      (*optlist[i].o_getfunc)(optlist[i].o_opt, optscr);
    }
  }

  /* Switch back to original screen */
  wmove(optscr, LINES - 1, 0);
  delwin(optscr);
  clearok(curscr, true);
  touchwin(stdscr);
  msg("");
  return false;
}

static bool
get_bool(void *vp, WINDOW *win)
{
  wrefresh(win);
  switch (readchar())
  {
    case 't': case 'T':
      *(bool *)vp = true;
      waddstr(win, "True ");
      return 0;

    case 'f': case 'F':
      *(bool *)vp = false;
      waddstr(win, "False");
      return 0;

    case '\n': case '\r': case KEY_ESCAPE: return 1;

    default: return get_bool(vp, win);
  }
}

static bool
get_sf(void *vp, WINDOW *win)
{
  bool was_sf = see_floor;

  if (get_bool(vp, win) != 0)
    return 1;
  else if (was_sf == see_floor)
    return 0;

  if (!see_floor) {
    see_floor = true;
    erase_lamp(&hero, proom);
    see_floor = false;
  }
  else
    look(false);

  return 0;
}


/* TODO: Move this to io.c */
bool
get_str(void *vopt, WINDOW *win)
{
  char buf[MAXSTR];
  signed char c = ~KEY_ESCAPE;
  unsigned i = strlen((char *) vopt);
  int oy, ox;

  getyx(win, oy, ox);

  strucpy(buf, (char *) vopt, i);
  waddstr(win, buf);

  /* loop reading in the string, and put it in a temporary buffer */
  while (c != KEY_ESCAPE)
  {
    wrefresh(win);
    c = readchar();

    if (c == '\n' || c == '\r' || c == -1)
      break;

    else if (c == erasechar() && i > 0)
    {
      i--;
      wmove(win, oy, ox + i);
      wclrtoeol(win);
    }

    else if (c == killchar())
    {
      i = 0;
      wmove(win, oy, ox);
      wclrtoeol(win);
    }

    else if (c == '~' && i == 0)
    {
      strcpy(buf, md_gethomedir());
      waddstr(win, md_gethomedir());
      i += strlen(md_gethomedir());
    }

    else if (i < MAXINP && (isprint(c) || c == ' '))
    {
      buf[i++] = c;
      waddch(win, c);
    }
  }

  buf[i] = '\0';
  if (i > 0) /* only change option if something has been typed */
    strucpy((char *) vopt, buf, (int) strlen(buf));
  else
    waddstr(win, vopt);
  if (win == stdscr)
    mpos += i;

  wrefresh(win);
  return c == KEY_ESCAPE ? 1 : 0;
}

