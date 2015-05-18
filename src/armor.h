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

const char *armor_name(enum armor_t armor);
int armor_ac(enum armor_t armor);
int armor_value(enum armor_t armor);
int armor_probability(enum armor_t i);
enum armor_t armor_type_random(void);

int armor_for_thing(THING *thing); /* Returns the AC of a creature */
void armor_rust(void);             /* Rust players armor */
bool armor_command_wear(void);     /* Let player select something to wear */

#endif /* _ROGUE14_ARMOR_H_ */
