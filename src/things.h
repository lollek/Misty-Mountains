#pragma once

#include <list>
#include <vector>
#include <string>

#include "Coordinate.h"
#include "rooms.h"
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

#define GOLDCALC	(os_rand_range(50 + 10 * level) + 2)

extern std::vector<obj_info> things;

/* Return the name of something as it would appear in an inventory. */
char* inv_name(char* buf, Item const* item, bool drop);

/* Put something down */
bool drop(void);

/* Return a new thing */
Item* new_thing(void);
Item* new_amulet(void);
Item* new_food(int which);

/* Pick an item out of a list of nitems possible objects */
size_t pick_one(std::vector<obj_info>& start, size_t nitems);

/* list what the player has discovered in this game of a certain type */
void discovered(void);
