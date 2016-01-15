#pragma once

#include "things.h"

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
  char const* name;
  int ac;    /* Armor bonus */
  int prob;  /* probability of drop */
  int value; /* Value in gold */
  int known; /* Is it known by player? */
};

char const* armor_name(enum armor_t armor);
int armor_ac(enum armor_t armor);
int armor_value(enum armor_t armor);
int armor_probability(enum armor_t i);
enum armor_t armor_type_random();

int armor_for_monster(monster const* mon); /* Returns the AC of a creature */
void armor_rust();             /* Rust players armor */
bool armor_command_wear();     /* Let player select something to wear */

void armor_description(Item const* item, char* buf);

Item* armor_create(int which, int random_stats);
