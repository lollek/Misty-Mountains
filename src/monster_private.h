#pragma once

#include <string>
#include <list>
#include <vector>

#include "Coordinate.h"
#include "things.h"

extern std::vector<monster_template> const monsters;

/* Find proper destination for monster */
void monster_find_new_target(Monster* tp);

void monster_set_target(Monster* mon, Coordinate const& target);
