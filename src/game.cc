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

IO*    Game::io = nullptr;
Level* Game::level = nullptr;
int    Game::current_level = 1;
int    Game::levels_without_food = 0;
int    Game::max_level_visited = 1;

void
Game::new_level(int dungeon_level) {

  /* Set max level we've been to */
  Game::current_level = dungeon_level;
  if (Game::current_level > Game::max_level_visited) {
    Game::max_level_visited = Game::current_level;
  }

  if (Game::level != nullptr) {
    delete Game::level;
  }

  Game::level = new Level();

  // Drop player in the new dungeon level
  if (player == nullptr) {
    player = new Player();
  }

  Coordinate new_player_pos = player->get_position();
  Game::level->get_random_room_coord(nullptr, &new_player_pos, 0, true);
  player->set_position(new_player_pos);
  room_enter(new_player_pos);
  Game::io->print_color(new_player_pos.x, new_player_pos.y, PLAYER);

  // Unhold player just in case
  player->set_not_held();

  // Reapply hallucination, just in case
  if (player->is_hallucinating()) {
    daemon_change_visuals();
  }
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

  Game::io = new IO();

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

  /* Init stuff */
  Scroll::init_scrolls();               // Set up names of scrolls
  Potion::init_potions();               // Set up colors of potions
  Ring::init_rings();                   // Set up stone settings of rings
  Wand::init_wands();                   // Set up materials of wands
  Game::new_level(Game::current_level); // Set up level (and player)

  /* Start up daemons and fuses */
  daemon_start(runners_move, AFTER);
  daemon_start(doctor, AFTER);
  daemon_start(ring_abilities, AFTER);
}

Game::~Game() {
  delete Game::io;
  Game::io = nullptr;
}
