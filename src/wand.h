#ifndef _ROGUE14_WAND_H_
#define _ROGUE14_WAND_H_

#include <stdbool.h>

#include "rogue.h"

enum wand
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
void *__wands_ptr(void);

/* Sets up wands for use
 * wand_init or wand_load_state should run before wands are used */
void wand_init(void);
bool wand_load_state(void *fd);

/* Save wand state to file */
bool wand_save_state(void *fd);

/* Returns the wand's material as a string */
const char *wand_material(enum wand wand);

/* Returns a description of the obj (e.g. for inventory screen) */
char *wand_description(THING *obj, char *buf);

/* What the hero has called the wand (might be NULL) */
const char *wand_nickname(THING *obj);

/* Does the player know what the wand does? */
void wand_set_known(enum wand wand);
bool wand_is_known(enum wand wand);

/* How mmuch gold is the wand worth? */
int wand_get_worth(enum wand wand);

/* Set name of wand */
void wand_set_name(enum wand wand, const char *new_name);

/* Set up a new wand */
THING *wand_create(int which);

/* Perform a zap with a wand */
bool wand_zap(void);

/* Fire a bolt in a given direction from a specific starting place */
void fire_bolt(coord *start, coord *dir, char *name);


#endif /* _ROGUE14_WAND_H_ */
