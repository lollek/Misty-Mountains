#pragma once

#include <vector>

#include "damage.h"

struct attack_modifier {
  attack_modifier();
  attack_modifier(attack_modifier const&) = default;

  ~attack_modifier() = default;

  attack_modifier& operator=(attack_modifier const&) = default;
  attack_modifier& operator=(attack_modifier&&) = default;

  void add_strength_modifiers(int strength);

  int                 to_hit;
  int                 to_dmg;
  std::vector<damage> damage;
};


