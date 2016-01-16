#pragma once

#include <string>

#include "monster.h"
#include "item.h"

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
  std::string const name;
  int ac;    /* Armor bonus */
  int prob;  /* probability of drop */
  int value; /* Value in gold */
  int known; /* Is it known by player? */
};

std::string const& armor_name(enum armor_t armor);
int armor_ac(enum armor_t armor);
int armor_value(enum armor_t armor);
int armor_probability(enum armor_t i);
enum armor_t armor_type_random();

void armor_rust();             /* Rust players armor */
bool armor_command_wear();     /* Let player select something to wear */

std::string armor_description(Item const* item);

Item* armor_create(int which, int random_stats);
