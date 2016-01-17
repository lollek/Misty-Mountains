#include <string>
#include <iostream>

using namespace std;

#include "os.h"
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

#include "Game.h"

Level* Game::level = nullptr;
int    Game::current_level = 1;

void
Game::new_level(int dungeon_level) {
  if (Game::level != nullptr) {
    delete Game::level;
  }

  Game::level = new Level(dungeon_level);
}

int Game::init_graphics()
{
  initscr();  /* Start up cursor package */

  /* Ncurses colors */
  if (use_colors) {
    if (start_color() == ERR) {
      endwin();
      cerr
        << "Error: Failed to start colors. "
        << "Try restarting without colors enabled\n";
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

  if (LINES < NUMLINES || COLS < NUMCOLS) {
    endwin();
    cerr << "\nSorry, the screen must be at least "
         << NUMLINES << "x" << NUMCOLS << "\n";
    return 1;
  }

  raw();     /* Raw mode */
  noecho();  /* Echo off */
  hw = newwin(LINES, COLS, 0, 0);

  return 0;
}

Game::Game() {

  /* Parse environment opts */
  string name = os_whoami();
  strucpy(whoami, name.c_str(), name.size());

  cout << "Hello " << whoami << ", just a moment while I dig the dungeon..." << flush;

  /* Init Graphics */
  if (init_graphics() != 0)
    exit(1);
  idlok(stdscr, true);
  idlok(hw, true);

  /* Init stuff */
  player = new Player();
  scroll_init();                        /* Set up names of scrolls */
  potions_init();                       /* Set up colors of potions */
  ring_init();                          /* Set up stone settings of rings */
  wand_init();                          /* Set up materials of wands */

  Game::new_level(Game::current_level);

  /* Start up daemons and fuses */
  daemon_start(daemon_runners_move, 0, AFTER);
  daemon_start(daemon_doctor, 0, AFTER);
  daemon_start(daemon_ring_abilities, 0, AFTER);
}

