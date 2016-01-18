#pragma once

#include <vector>

#include "damage.h"

struct attack_modifier {
  attack_modifier() : to_hit(0), to_dmg(0), damage({}) {}
  attack_modifier(attack_modifier const&) = default;

  ~attack_modifier() = default;

  attack_modifier& operator=(attack_modifier const&) = default;
  attack_modifier& operator=(attack_modifier&&) = default;

  int            to_hit;
  int            to_dmg;
  vector<damage> damage;
};


