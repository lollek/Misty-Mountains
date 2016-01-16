#pragma once

class Game {
public:
  Game();
  Game(Game const&) = delete;

  ~Game() = default;

  Game& operator=(Game const&) = delete;

private:
  int init_graphics();
};
