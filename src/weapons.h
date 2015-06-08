#ifndef _ROGUE14_WEAPON_H_
#define _ROGUE14_WEAPON_H_

#include <stdbool.h>

#include "rogue.h"

enum weapon_type
{
  MACE     = 0,
  SWORD    = 1,
  BOW      = 2,
  ARROW    = 3,
  DAGGER   = 4,
  TWOSWORD = 5,
  DART     = 6,
  SHIRAKEN = 7,
  SPEAR    = 8,
  MAXWEAPONS
};
#define FLAME MAXWEAPONS /* fake entry for dragon breath (ick) */

/* TODO: Remove vars */
extern struct obj_info weap_info[MAXWEAPONS +1];

bool weapons_save_state(void);
bool weapons_load_state(void);

/* Do the actual motion on the screen done by an object traveling
 * across the room */
void do_motion(THING* obj, int ydelta, int xdelta);

/* Drop an item someplace around here. */
void fall(THING* obj, bool pr);

THING* weapon_create(int which, bool random_stats);

bool weapon_wield(THING* weapon);
void set_last_weapon(THING* weapon);
bool last_weapon(void);

/* Pick a random position around the give (y, x) coordinates */
bool fallpos(coord const* pos, coord* newpos);

void weapon_description(THING* obj, char* buf);

#endif /* _ROGUE14_WEAPON_H_ */
