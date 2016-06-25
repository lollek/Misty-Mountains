#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>

#include "colors.h"
#include "command.h"
#include "daemons.h"
#include "disk.h"
#include "error_handling.h"
#include "io.h"
#include "item/armor.h"
#include "item/potions.h"
#include "item/rings.h"
#include "item/scrolls.h"
#include "item/wand.h"
#include "item/weapons.h"
#include "level.h"
#include "misc.h"
#include "options.h"
#include "os.h"
#include "player.h"
#include "rogue.h"
#include "traps.h"

#include "game.h"

using namespace std;

namespace Game {

namespace {

bool has_been_initialized{false};
unsigned starting_seed{0};

unsigned long long constexpr TAG_GAME{0x6000000000000000ULL};
unsigned long long constexpr TAG_WHOAMI{0x6000000000000001ULL};
unsigned long long constexpr TAG_SAVEPATH{0x6000000000000002ULL};
unsigned long long constexpr TAG_LEVEL{0x6000000000000003ULL};
unsigned long long constexpr TAG_FOODLESS{0x6000000000000004ULL};

}  // anonymous namespace

int const amulet_min_level{26};

IO* io{nullptr};
Level* level{nullptr};
string* whoami{nullptr};
string* save_game_path{nullptr};
int current_level{0};
int levels_without_food{0};

void initialize(string const& whoami_, string const& save_path_) {
  if (has_been_initialized) {
    error("Game initializer already started");
  }
  has_been_initialized = true;

  starting_seed = os_rand_seed;
  whoami = new string(whoami_);
  save_game_path = new string(save_path_);
  current_level = 1;

  io = new IO();
  Scroll::init_scrolls();    // Names of scrolls
  Color::init_colors();      // Colors for potions and stuff
  Potion::init_potions();    // Colors of potions
  Ring::init_rings();        // Stone settings of rings
  Wand::init_wands();        // Materials of wands
  Daemons::init_daemons();   // Over-time-effects
  Trap::init_traps();        // Trap types
  io->character_creation();  // New player
  new_level(current_level);  // New Level

  // Start up daemons and fuses
  Daemons::daemon_start(Daemons::runners_move, AFTER);
  Daemons::daemon_start(Daemons::doctor, AFTER);
  Daemons::daemon_start(Daemons::ring_abilities, AFTER);
}

void initialize(istream& savefile) {
  if (has_been_initialized) {
    error("Game initializer already started");
  }
  has_been_initialized = true;

  io = new IO();

  try {
    Scroll::load_scrolls(savefile);
    Color::init_colors();
    Potion::load_potions(savefile);
    Ring::load_rings(savefile);
    Wand::load_wands(savefile);
    Daemons::load_daemons(savefile);
    Trap::init_traps();
    Player::load_player(savefile);
    // level = new Level(savefile);

    Disk::load_tag(TAG_GAME, savefile);

    Disk::load(TAG_WHOAMI, whoami, savefile);
    Disk::load(TAG_SAVEPATH, save_game_path, savefile);
    Disk::load(TAG_LEVEL, current_level, savefile);
    Disk::load(TAG_FOODLESS, levels_without_food, savefile);

    Game::new_level(1);

    Disk::load_tag(TAG_GAME, savefile);
  } catch (fatal_error& e) {
    io->stop_curses();
    cout << "Failed to load game, corrupt save file (" << e.what() << ")\n";
    exit();
  }
}

void exit() {
  Trap::free_traps();
  Daemons::free_daemons();
  Wand::free_wands();
  Ring::free_rings();
  Potion::free_potions();
  Color::free_colors();
  Scroll::free_scrolls();

  Player::free_player();

  if (io != nullptr) {
    delete io;
    io = nullptr;
  }

  if (whoami != nullptr) {
    delete whoami;
    whoami = nullptr;
  }

  if (save_game_path != nullptr) {
    delete save_game_path;
    save_game_path = nullptr;
  }

  ::exit(0);
}

void new_level(int dungeon_level) {
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

int run() {
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
  } catch (const std::runtime_error& ex) {
    endwin();
    cout << ex.what() << endl;
    return 1;
  }
#else
  for (;;) command();
#endif

  // CODE NOT REACHED
}

bool save() {
  ofstream savefile{*save_game_path, fstream::out | fstream::trunc};
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
  // level->save(savefile);

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
  Ring::test_rings();
  Wand::test_wands();
  Daemons::test_daemons();
  Player::test_player();
#endif  // NDEBUG
  return true;
}

}  // namespace Game
