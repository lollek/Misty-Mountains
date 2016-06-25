#pragma once

#include <istream>
#include <ostream>
#include <string>

#include "io.h"
#include "level.h"

namespace Game {
void initialize(std::string const& whoami, std::string const& save_path);
void initialize(std::istream& savefile);

int run();

void exit() __attribute__((noreturn));
void new_level(int dungeon_level);
bool save();

extern IO* io;
extern Level* level;
extern std::string* whoami;
extern std::string* save_game_path;
extern int current_level;
extern int const amulet_min_level;
extern int levels_without_food;
};
