#pragma once

#include <vector>
#include <string>

#include "rogue.h"

enum wand_t
{
  WS_LIGHT     = 0,
  WS_INVIS     = 1,
  WS_ELECT     = 2,
  WS_FIRE      = 3,
  WS_COLD      = 4,
  WS_POLYMORPH = 5,
  WS_MISSILE   = 6,
  WS_HASTE_M   = 7,
  WS_SLOW_M    = 8,
  WS_DRAIN     = 9,
  WS_NOP       = 10,
  WS_TELAWAY   = 11,
  WS_TELTO     = 12,
  WS_CANCEL    = 13,
  MAXSTICKS
};

extern std::vector<obj_info> wands_info;

/* Sets up wands for use
 * wand_init or wand_load_state should run before wands are used */
void wand_init();

/* Returns the wand's material as a string */
std::string const& wand_material(enum wand_t wand);

/* Returns a description of the obj (e.g. for inventory screen) */
char* wand_description(Item const* item, char* buf);

/* Does the player know what the wand does? */
void wand_set_known(enum wand_t wand);
bool wand_is_known(enum wand_t wand);

/* How mmuch gold is the wand worth? */
size_t wand_get_worth(enum wand_t wand);

/* Set name of wand */
void wand_set_name(enum wand_t wand, std::string const& new_name);

/* Set up a new wand */
Item* wand_create(int which);

/* Perform a zap with a wand */
bool wand_zap();
