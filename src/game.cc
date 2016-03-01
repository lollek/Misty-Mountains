#include <string>
#include <iostream>
#include <istream>
#include <ostream>
#include <fstream>

#include "disk.h"
#include "command.h"
#include "error_handling.h"
#include "os.h"
#include "potions.h"
#include "scrolls.h"
#include "io.h"
#include "armor.h"
#include "daemons.h"
#include "colors.h"
#include "level.h"
#include "rings.h"
#include "misc.h"
#include "player.h"
#include "weapons.h"
#include "wand.h"
#include "options.h"
#include "rogue.h"
#include "traps.h"

#include "game.h"

using namespace std;

Game*   Game::game_ptr = nullptr;
IO*     Game::io = nullptr;
Level*  Game::level = nullptr;
string* Game::whoami = nullptr;
string* Game::save_game_path = nullptr;
int     Game::current_level = 1;
int     Game::levels_without_food = 0;

void Game::exit() {
  if (game_ptr != nullptr) {
    delete game_ptr;
  }
  ::exit(0);
}

void Game::new_level(int dungeon_level) {

  current_level = dungeon_level;

  if (level != nullptr) {
    delete level;
  }

  level = new Level();

  Coordinate new_player_pos = player->get_position();
  level->get_random_room_coord(nullptr, &new_player_pos, 0, true);
  player->set_position(new_player_pos);

  // Unhold player just in case
  player->set_not_held();
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
  io->message("Seed: #" + to_string(starting_seed));
#endif

  player->set_previous_room(level->get_room(player->get_position()));

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

Game::Game(string const& whoami_, string const& save_path_)
  : starting_seed(os_rand_seed) {
    whoami = new string(whoami_);
    save_game_path = new string(save_path_);

  if (game_ptr != nullptr) {
    error("Game is a singleton class");
  }
  game_ptr = this;

  // Init stuff
  io = new IO();                        // Graphics
  Scroll::init_scrolls();               // Names of scrolls
  Color::init_colors();                 // Colors for potions and stuff
  Potion::init_potions();               // Colors of potions
  Ring::init_rings();                   // Stone settings of rings
  Wand::init_wands();                   // Materials of wands
  Daemons::init_daemons();              // Over-time-effects
  Monster::init_monsters();             // Monster types
  Trap::init_traps();                   // Trap types
  player = new Player(true);            // New player
  new_level(current_level);             // New Level

  // Start up daemons and fuses
  Daemons::daemon_start(Daemons::runners_move, AFTER);
  Daemons::daemon_start(Daemons::doctor, AFTER);
  Daemons::daemon_start(Daemons::ring_abilities, AFTER);

  // Run the character creator
  io->character_creation();
}

Game::~Game() {
  Trap::free_traps();
  Monster::free_monsters();
  Daemons::free_daemons();
  Wand::free_wands();
  Ring::free_rings();
  Potion::free_potions();
  Color::free_colors();
  Scroll::free_scrolls();

  delete io;
  io = nullptr;

  delete whoami;
  whoami = nullptr;
}


Game::Game(istream& savefile) {

  if (game_ptr != nullptr) {
    error("Game is a singleton class");
  }
  game_ptr = this;

  io = new IO();

  try {
    Scroll::load_scrolls(savefile);
    Color::init_colors();
    Potion::load_potions(savefile);
    Ring::load_rings(savefile);
    Wand::load_wands(savefile);
    Daemons::load_daemons(savefile);
    Monster::init_monsters();
    Trap::init_traps();
    Player::load_player(savefile);
    //level = new Level(savefile);

    Disk::load_tag(TAG_GAME, savefile);

    Disk::load(TAG_WHOAMI, whoami, savefile);
    Disk::load(TAG_SAVEPATH, save_game_path, savefile);
    Disk::load(TAG_LEVEL, current_level, savefile);
    Disk::load(TAG_FOODLESS, levels_without_food, savefile);

    Game::new_level(1);


    Disk::load_tag(TAG_GAME, savefile);
  } catch (fatal_error &e) {
    io->stop_curses();
    cout << "Failed to load game, corrupt save file (" << e.what() << ")\n";
    exit();
  }

}

bool Game::save() {
  ofstream savefile(*save_game_path, fstream::out | fstream::trunc);
  if (!savefile) {
    io->message("Failed to save file " + *save_game_path);
    return false;
  }

  Scroll::save_scrolls(savefile);
  Potion::save_potions(savefile);
  Ring::save_rings(savefile);
  Wand::save_wands(savefile);
  Daemons::save_daemons(savefile);
  Player::save_player(savefile);
  //level->save(savefile);

  Disk::save_tag(TAG_GAME, savefile);

  Disk::save(TAG_WHOAMI, whoami, savefile);
  Disk::save(TAG_SAVEPATH, save_game_path, savefile);
  Disk::save(TAG_LEVEL, current_level, savefile);
  Disk::save(TAG_FOODLESS, levels_without_food, savefile);

  Disk::save_tag(TAG_GAME, savefile);

  savefile.close();

#ifndef NDEBUG
  Scroll::test_scrolls();
  Potion::test_potions();
#endif //NDEBUG
  return true;
}
