#pragma once

#include <string>

#include "Coordinate.h"

/* Fire a bolt in a given direction from a specific starting place */
void magic_bolt(Coordinate* start, Coordinate* dir, std::string const& name);
