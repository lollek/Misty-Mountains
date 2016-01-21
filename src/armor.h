#pragma once

#include <string>

#include "monster.h"
#include "item.h"


class Armor : public Item {
public:
  enum Type {
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

  // Armor of given type
  Armor(Type type, bool random_stats);
  // Armor of random type
  Armor(bool random_stats);

};

struct armor_info_t
{
  std::string const name;
  int ac;    /* Armor bonus */
  int prob;  /* probability of drop */
  int value; /* Value in gold */
  int known; /* Is it known by player? */
};

std::string const& armor_name(Armor::Type armor);
int armor_ac(Armor::Type armor);
int armor_value(Armor::Type armor);
int armor_probability(Armor::Type i);
Armor::Type armor_type_random();

void armor_rust();             /* Rust players armor */
bool armor_command_wear();     /* Let player select something to wear */

std::string armor_description(Item const* item);
