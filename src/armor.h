#ifndef _ROGUE14_ARMOR_H_
#define _ROGUE14_ARMOR_H_

#include "rogue.h"

enum armor_t
{
  LEATHER = 0,
  RING_MAIL = 1,
  STUDDED_LEATHER = 2,
  SCALE_MAIL = 3,
  CHAIN_MAIL = 4,
  SPLINT_MAIL = 5,
  BANDED_MAIL = 6,
  PLATE_MAIL = 7,
  NARMORS
};

extern int a_class[NARMORS]; /* Armor class for each armor type */
extern struct obj_info arm_info[NARMORS]; /* Info for all armor */

int get_ac(THING *thing); /* Returns the AC of a creature */
void rust_players_armor(void); /* Rust players armor */
bool take_off(void);      /* Take off player's armor */
bool wear(void);          /* Let player select something to wear */

#endif /* _ROGUE14_ARMOR_H_ */
