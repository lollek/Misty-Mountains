#ifndef ROGUE14_WAND_H
#define ROGUE14_WAND_H

#include <stdbool.h>

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

/* TODO: Remove this */
void* __wands_ptr(void);

/* Sets up wands for use
 * wand_init or wand_load_state should run before wands are used */
void wand_init(void);
bool wand_load_state(void);

/* Save wand state to file */
bool wand_save_state(void);

/* Returns the wand's material as a string */
char const* wand_material(enum wand_t wand);

/* Returns a description of the obj (e.g. for inventory screen) */
char* wand_description(item const* item, char* buf);

/* Does the player know what the wand does? */
void wand_set_known(enum wand_t wand);
bool wand_is_known(enum wand_t wand);

/* How mmuch gold is the wand worth? */
int wand_get_worth(enum wand_t wand);

/* Set name of wand */
void wand_set_name(enum wand_t wand, char const* new_name);

/* Set up a new wand */
item* wand_create(int which);

/* Perform a zap with a wand */
bool wand_zap(void);

#endif /* ROGUE14_WAND_H */
