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

#include "Coordinate.h"

#include "io.h"
#include "misc.h"
#include "player.h"

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

bool option_autopickup(int type)
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
  io_debug_fatal("option_autopickup: unknown type: %d", type);
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
  return io_wreadstr(win, buf);
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
    {'1',    "Flush typeahead during battle?....", &fight_flush,    option::BOOL},
    {'2',    "Show position only at end of run?.", &jump,           option::BOOL},
    {'3',    "Follow turnings in passageways?...", &passgo,         option::BOOL},
    {POTION, "Pick up potions?..................", &pickup_potions, option::BOOL},
    {SCROLL, "Pick up scrolls?..................", &pickup_scrolls, option::BOOL},
    {FOOD,   "Pick up food?.....................", &pickup_food,    option::BOOL},
    {WEAPON, "Pick up weapons?..................", &pickup_weapons, option::BOOL},
    {ARMOR,  "Pick up armor?....................", &pickup_armor,   option::BOOL},
    {RING,   "Pick up rings?....................", &pickup_rings,   option::BOOL},
    {STICK,  "Pick up sticks?...................", &pickup_sticks,  option::BOOL},
    {AMMO,   "Pick up ammo?.....................", &pickup_ammo,    option::BOOL},
    {'4',    "Name..............................", whoami,          option::STR},
  };
  int const NOPTS = (sizeof optlist / sizeof (*optlist));
  char const* query = "Which value do you want to change? (ESC to exit) ";
  Coordinate const msg_pos (static_cast<int>(strlen(query)), 0);
  io_msg(query);

  WINDOW *optscr = NULL;
  optscr = dupwin(stdscr);

  /* Display current values of options */
  wmove(optscr, 1, 0);
  for (int i = 0; i < NOPTS; ++i)
  {
    wprintw(optscr, "%c) %s", optlist[i].index, optlist[i].o_prompt);
    if (optlist[i].put_type == option::BOOL)
      waddstr(optscr, optlist[i].o_opt ? "True" : "False");
    else if (optlist[i].put_type == option::STR)
      waddstr(optscr, static_cast<char *>(optlist[i].o_opt));
    waddch(optscr, '\n');
  }

  /* Loop and change values */
  char c = static_cast<char>(~KEY_ESCAPE);
  while (c != KEY_ESCAPE)
  {
    wmove(optscr, msg_pos.y, msg_pos.x);
    wrefresh(optscr);
    c = io_readchar(true);
    for (int i = 0; i < NOPTS; ++i)
      if (c == optlist[i].index)
      {
        wmove(optscr, i + 1, static_cast<int>(strlen(optlist[i].o_prompt) + 3));
        switch (optlist[i].put_type)
        {
          case option::BOOL: get_bool(static_cast<bool*>(optlist[i].o_opt), optscr); break;
          case option::STR:  get_str(static_cast<char*>(optlist[i].o_opt), optscr); break;
        }
        break;
      }
  }

  /* Switch back to original screen */
  wmove(optscr, LINES - 1, 0);
  delwin(optscr);
  clearok(curscr, true);
  touchwin(stdscr);
  io_msg_clear();
  return false;
}

