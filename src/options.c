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
#include <assert.h>

#include "io.h"
#include "misc.h"
#include "player.h"
#include "rogue.h"

#include "options.h"

bool fight_flush  = false;
bool jump         = true;
bool passgo       = false;
bool use_colors   = true;

static bool pickup_potions = true;
static bool pickup_scrolls = true;
static bool pickup_food    = true;
static bool pickup_weapons = false;
static bool pickup_armor   = false;
static bool pickup_rings   = true;
static bool pickup_sticks  = true;
static bool pickup_ammo    = true;

bool autopickup(int type)
{
  switch (type)
  {
    case AMMO:   return pickup_ammo;
    case POTION: return pickup_potions;
    case SCROLL: return pickup_scrolls;
    case FOOD:   return pickup_food;
    case WEAPON: return pickup_weapons;
    case ARMOR:  return pickup_armor;
    case RING:   return pickup_rings;
    case STICK:  return pickup_sticks;
    case AMULET: return true;
  }
  (void)fail("autopickup: unknown type: %d", type);
  assert(0);
  return false;
}

static bool
get_bool(bool* b, WINDOW* win)
{
  wrefresh(win);
  *b = !*b;
  waddstr(win, *b ? "True " : "False");
  return 0;
}

static inline bool
get_str(char* buf, WINDOW* win)
{
  return wreadstr(win, buf);
}

/** option:
 * Print and then set options from the terminal */
bool
option(void)
{
  struct option {
    char index;           /* What to press to change option */
    char const* o_prompt; /* prompt for interactive entry */
    void* o_opt;          /* pointer to thing to set function to print value */
    enum put_t { BOOL, STR } put_type;
  } optlist[] = {
    {'1',    "Flush typeahead during battle?....", &fight_flush,    BOOL},
    {'2',    "Show position only at end of run?.", &jump,           BOOL},
    {'3',    "Follow turnings in passageways?...", &passgo,         BOOL},
    {POTION, "Pick up potions?..................", &pickup_potions, BOOL},
    {SCROLL, "Pick up scrolls?..................", &pickup_scrolls, BOOL},
    {FOOD,   "Pick up food?.....................", &pickup_food,    BOOL},
    {WEAPON, "Pick up weapons?..................", &pickup_weapons, BOOL},
    {ARMOR,  "Pick up armor?....................", &pickup_armor,   BOOL},
    {RING,   "Pick up rings?....................", &pickup_rings,   BOOL},
    {STICK,  "Pick up sticks?...................", &pickup_sticks,  BOOL},
    {AMMO,   "Pick up ammo?.....................", &pickup_ammo,    BOOL},
    {'4',    "Name..............................", whoami,          STR},
    {'5',    "Save file.........................", file_name,       STR},
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
  wmove(optscr, 1, 0);
  for (int i = 0; i < NOPTS; ++i)
  {
    wprintw(optscr, "%c) %s", optlist[i].index, optlist[i].o_prompt);
    if (optlist[i].put_type == BOOL)
      waddstr(optscr, *(bool *) optlist[i].o_opt ? "True" : "False");
    else if (optlist[i].put_type == STR)
      waddstr(optscr, (char *) optlist[i].o_opt);
    waddch(optscr, '\n');
  }

  /* Loop and change values */
  char c = (char)~KEY_ESCAPE;
  while (c != KEY_ESCAPE)
  {
    wmove(optscr, msg_pos.y, msg_pos.x);
    wrefresh(optscr);
    c = readchar(true);
    for (int i = 0; i < NOPTS; ++i)
      if (c == optlist[i].index)
      {
        wmove(optscr, i + 1, (int)strlen(optlist[i].o_prompt) + 3);
        switch (optlist[i].put_type)
        {
          case BOOL: get_bool(optlist[i].o_opt, optscr); break;
          case STR:  get_str(optlist[i].o_opt, optscr); break;
        }
        break;
      }
  }

  /* Switch back to original screen */
  wmove(optscr, LINES - 1, 0);
  delwin(optscr);
  clearok(curscr, true);
  touchwin(stdscr);
  clearmsg();
  return false;
}

