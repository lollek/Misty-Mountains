#ifndef _ROGUE14_WEAPON_H_
#define _ROGUE14_WEAPON_H_

#include <stdbool.h>

#include "rogue.h"

#define FLAME MAXWEAPONS /* fake entry for dragon breath (ick) */
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

/* TODO: Remove vars */
extern struct obj_info weap_info[MAXWEAPONS +1];

bool weapons_save_state(void);
bool weapons_load_state(void);

/* Fire a missile in a given direction */
bool missile(int ydelta, int xdelta);

/* Do the actual motion on the screen done by an object traveling
 * across the room */
void do_motion(THING *obj, int ydelta, int xdelta);

/* Drop an item someplace around here. */
void fall(THING *obj, bool pr);

/* Set up the initial goodies for a weapon */
void init_weapon(THING *weap, int which);

/* Does the missile hit the monster? */
int hit_monster(int y, int x, THING *obj);

/* Figure out the plus number for armor/weapons */
char *num(int n1, int n2, char type);

bool weapon_wield(THING *weapon);
void set_last_weapon(THING *weapon);
bool last_weapon(void);

/* Pick a random position around the give (y, x) coordinates */
bool fallpos(coord *pos, coord *newpos);

#endif /* _ROGUE14_WEAPON_H_ */
