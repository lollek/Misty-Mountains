#include "attack_modifier.h"

attack_modifier::attack_modifier() : to_hit{0}, to_dmg{0}, damage{{}} {}

void attack_modifier::add_strength_modifiers(int strength) {
  switch (strength) {
    case 0:
      to_hit -= 7;
      to_dmg -= 7;
      return;
    case 1:
      to_hit -= 6;
      to_dmg -= 6;
      return;
    case 2:
      to_hit -= 5;
      to_dmg -= 5;
      return;
    case 3:
      to_hit -= 4;
      to_dmg -= 4;
      return;
    case 4:
      to_hit -= 3;
      to_dmg -= 3;
      return;
    case 5:
      to_hit -= 2;
      to_dmg -= 2;
      return;
    case 6:
      to_hit -= 1;
      to_dmg -= 1;
      return;

    default: return;

    case 16: to_dmg += 1; return;
    case 17:
      to_hit += 1;
      to_dmg += 1;
      return;
    case 18:
      to_hit += 1;
      to_dmg += 2;
      return;
    case 19:
      to_hit += 1;
      to_dmg += 3;
      return;
    case 20:
      to_hit += 1;
      to_dmg += 3;
      return;
    case 21:
      to_hit += 2;
      to_dmg += 4;
      return;
    case 22:
      to_hit += 2;
      to_dmg += 5;
      return;
    case 23:
      to_hit += 2;
      to_dmg += 5;
      return;
    case 24:
      to_hit += 2;
      to_dmg += 5;
      return;
    case 25:
      to_hit += 2;
      to_dmg += 5;
      return;
    case 26:
      to_hit += 2;
      to_dmg += 5;
      return;
    case 27:
      to_hit += 2;
      to_dmg += 5;
      return;
    case 28:
      to_hit += 2;
      to_dmg += 5;
      return;
    case 29:
      to_hit += 2;
      to_dmg += 5;
      return;
    case 30:
      to_hit += 2;
      to_dmg += 5;
      return;
    case 31:
      to_hit += 3;
      to_dmg += 6;
      return;
  }
}
