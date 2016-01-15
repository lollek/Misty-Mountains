#pragma once

#include <vector>

#include "things.h" /* struct obj_info */

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

/* Variables, TODO: Make these private */
extern std::vector<obj_info> potion_info; /* A list of potions and info */

void potions_init(void);

bool potion_save_state(void);
bool potion_load_state(void);

/* Functions */
bool potion_quaff_something(void);  /* Quaff a potion from the pack */
void potion_description(item const* obj, char buf[]);

item* potion_create(int which);
