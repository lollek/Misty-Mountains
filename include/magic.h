#pragma once

#include <string>

#include "coordinate.h"

// Fire a bolt in a given direction from a specific starting place
void magic_bolt(Coordinate* start, Coordinate* dir, std::string const& name);

// Player specific magic
int magic_hold_nearby_monsters();  // Hold monsters in radius 2, returns num
                                   // affected
