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

/* TODO: Hide these vars */
struct obj_info ws_info[MAXSTICKS];
const char *ws_made[MAXSTICKS];		/* What sticks are made of */
char *ws_type[MAXSTICKS];		/* Is it a wand or a staff */
void *__wand_material_ptr(void);
size_t NMATERIAL;

/* Sets up wands for use */
void wand_init(void);

const char *wand_material(enum wand wand);

/* Set up a new stick */
void fix_stick(THING *cur);

/* Perform a zap with a wand */
bool do_zap(void);

/* Do drain hit points from player shtick */
void drain(void);

/* Fire a bolt in a given direction from a specific starting place */
void fire_bolt(coord *start, coord *dir, char *name);

/* Return an appropriate string for a wand charge */
char *charge_str(THING *obj);

#endif /* _ROGUE14_WAND_H_ */
