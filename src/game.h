#pragma once

#include <string>

#include "level.h"
#include "io.h"

class Game {
public:
  Game(std::string const& whoami);
  Game(Game const&) = delete;

  ~Game();

  Game& operator=(Game const&) = delete;

  int run();

  static void exit() __attribute__((noreturn));
  static void new_level(int dungeon_level);

  static IO*           io;
  static Level*        level;
  static std::string*  whoami;
  static int           current_level;
  static int constexpr amulet_min_level = 26;
  static int           levels_without_food;
  static int           max_level_visited;

private:
  static Game* game_ptr;
  unsigned     starting_seed;
};
