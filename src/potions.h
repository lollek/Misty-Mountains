#ifndef _ROGUE14_POTIONS_H_
#define _ROGUE14_POTIONS_H_

#include "rogue.h" /* struct obj_info */

/* Potion types */
enum potion_t
{
  P_CONFUSE  = 0,
  P_LSD      = 1,
  P_POISON   = 2,
  P_STRENGTH = 3,
  P_SEEINVIS = 4,
  P_HEALING  = 5,
  P_MFIND    = 6,
  P_TFIND    = 7,
  P_RAISE    = 8,
  P_XHEAL    = 9,
  P_HASTE    = 10,
  P_RESTORE  = 11,
  P_BLIND    = 12,
  P_LEVIT    = 13,
  NPOTIONS
};

/* Variables */
char *p_colors[NPOTIONS];           /* Colors of the potions */
struct obj_info pot_info[NPOTIONS]; /* A list of potions and info */

/* Functions */
bool is_quaffable(THING *thing);    /* Check if we can drink something */
bool quaff();                       /* Quaff a potion from the pack */
inline void learn_potion(enum potion_t potion); /* Learn what a potion does */
inline bool knows_potion(enum potion_t potion); /* She knows the potion? */

#endif /* _ROGUE14_POTIONS_H_ */
