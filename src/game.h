#pragma once

#include "level.h"

class Game {
public:
  Game();
  Game(Game const&) = delete;

  ~Game() = default;

  Game& operator=(Game const&) = delete;


  static void new_level(int dungeon_level);

  static Level* level;
  static int current_level;

private:
  int init_graphics();
};
