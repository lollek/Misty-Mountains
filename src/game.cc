#include <string>
#include <iostream>

using namespace std;

#include "command.h"
#include "error_handling.h"
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

Game*  Game::game_ptr = nullptr;
IO*    Game::io = nullptr;
Level* Game::level = nullptr;
int    Game::current_level = 1;
int    Game::levels_without_food = 0;
int    Game::max_level_visited = 1;

void Game::exit() {
  if (game_ptr != nullptr) {
    delete game_ptr;
  }
  ::exit(0);
}

void Game::new_level(int dungeon_level) {

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
  Game::io->print_color(new_player_pos.x, new_player_pos.y, player->get_type());

  // Unhold player just in case
  player->set_not_held();

  // Reapply hallucination, just in case
  if (player->is_hallucinating()) {
    daemon_change_visuals();
  }
}

int Game::run() {

  // Try to crash cleanly, and autosave if possible
  // Unless we are debugging, since that messes with gdb/lldb
#ifdef NDEBUG
  signal(SIGHUP, save_auto);
  signal(SIGQUIT, command_signal_endit);
  signal(SIGILL, save_auto);
  signal(SIGTRAP, save_auto);
  signal(SIGIOT, save_auto);
  signal(SIGFPE, save_auto);
  signal(SIGBUS, save_auto);
  signal(SIGSEGV, save_auto);
  signal(SIGSYS, save_auto);
  signal(SIGTERM, save_auto);
  signal(SIGINT, command_signal_quit);
#else
  Game::io->message("Seed: #" + to_string(os_rand_seed));
#endif

  player->set_previous_room(Game::level->get_room(player->get_position()));

#ifdef NDEBUG
  try {
    for (;;) command();
  } catch (const std::runtime_error &ex) {
    endwin();
    cout << ex.what() << endl;
    return 1;
  }
#else
  for (;;) command();
#endif

  // CODE NOT REACHED
}

Game::Game() {

  if (game_ptr != nullptr) {
    error("Game is a singleton class");
  }
  game_ptr = this;

  // Parse environment opts
  if (whoami.empty()) {
    whoami = os_whoami();
  }

  cout << "Hello " << whoami << ", just a moment while I dig the dungeon..." << flush;

  /* Init stuff */
  Game::io = new IO();                  // Set up graphics
  Scroll::init_scrolls();               // Set up names of scrolls
  Potion::init_potions();               // Set up colors of potions
  Ring::init_rings();                   // Set up stone settings of rings
  Wand::init_wands();                   // Set up materials of wands
  Game::new_level(Game::current_level); // Set up level (and player)

  // Start up daemons and fuses
  daemon_start(runners_move, AFTER);
  daemon_start(doctor, AFTER);
  daemon_start(ring_abilities, AFTER);
}

Game::~Game() {
  Ring::free_rings();
  delete Game::io;
}
