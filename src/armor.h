#ifndef _ROGUE14_ARMOR_H_
#define _ROGUE14_ARMOR_H_

#include "rogue.h"

int get_ac(THING *thing); /* Returns the AC of a creature */
bool wear(void);          /* Let player select something to wear */
bool take_off(void);      /* Take off player's armor */
void waste_time(void);    /* Run daemons */

#endif /* _ROGUE14_ARMOR_H_ */
