#pragma once

#include "level.h"

class Game {
public:
  Game();
  Game(Game const&) = delete;

  ~Game() = default;

  Game& operator=(Game const&) = delete;

  static Level* level;

private:
  int init_graphics();
};
