#pragma once

#include <list>
#include <vector>
#include <string>

#include "Coordinate.h"
#include "damage.h"
#include "item.h"

/* Stuff about objects */
struct obj_info {
  std::string oi_name;
  size_t      oi_prob;
  size_t      oi_worth;
  std::string oi_guess;
  bool        oi_know;
};

extern std::vector<obj_info> things;

/* Return a new thing */
Item* new_thing(void);

/* Pick an item out of a list of nitems possible objects */
size_t pick_one(std::vector<obj_info>& start, size_t nitems);
