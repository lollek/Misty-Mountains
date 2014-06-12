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

typedef struct
{
  int pa_flags;
  void (*pa_daemon)();
  int pa_time;
  char *pa_high, *pa_straight;
} PACT;

char *p_colors[NPOTIONS];           /* Colors of the potions */
struct obj_info pot_info[NPOTIONS]; /* A list of potions and info */

void quaff();                       /* Quaff a potion from the pack */
void do_pot(int type, bool knowit); /* Do a potion with standard setup, this
                                       means it uses a fuse and sets a flag */

#endif /* _ROGUE14_POTIONS_H_ */
