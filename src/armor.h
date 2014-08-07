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

struct armor_info_t
{
  const char *name;
  int ac;    /* Armor bonus */
  int prob;  /* probability of drop */
  int value; /* Value in gold */
  int known; /* Is it known by player? */
};

extern struct armor_info_t armors[NARMORS]; /* Info for all armor */

int get_ac(THING *thing); /* Returns the AC of a creature */
char random_armor_type(void); /* Returns one of armor_t */
void rust_players_armor(void); /* Rust players armor */
bool wear(void);          /* Let player select something to wear */

#endif /* _ROGUE14_ARMOR_H_ */
