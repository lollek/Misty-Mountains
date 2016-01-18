#include "attack_modifier.h"

attack_modifier::attack_modifier()
  : to_hit(0), to_dmg(0), damage({})
{}


void attack_modifier::add_strength_modifiers(int strength) {
  switch (strength) {

    case  0: this->to_hit -= 7; this->to_dmg -= 7; return;
    case  1: this->to_hit -= 6; this->to_dmg -= 6; return;
    case  2: this->to_hit -= 5; this->to_dmg -= 5; return;
    case  3: this->to_hit -= 4; this->to_dmg -= 4; return;
    case  4: this->to_hit -= 3; this->to_dmg -= 3; return;
    case  5: this->to_hit -= 2; this->to_dmg -= 2; return;
    case  6: this->to_hit -= 1; this->to_dmg -= 1; return;

    default: return;

    case 16:                  this->to_dmg += 1; return;
    case 17: this->to_hit += 1; this->to_dmg += 1; return;
    case 18: this->to_hit += 1; this->to_dmg += 2; return;
    case 19: this->to_hit += 1; this->to_dmg += 3; return;
    case 20: this->to_hit += 1; this->to_dmg += 3; return;
    case 21: this->to_hit += 2; this->to_dmg += 4; return;
    case 22: this->to_hit += 2; this->to_dmg += 5; return;
    case 23: this->to_hit += 2; this->to_dmg += 5; return;
    case 24: this->to_hit += 2; this->to_dmg += 5; return;
    case 25: this->to_hit += 2; this->to_dmg += 5; return;
    case 26: this->to_hit += 2; this->to_dmg += 5; return;
    case 27: this->to_hit += 2; this->to_dmg += 5; return;
    case 28: this->to_hit += 2; this->to_dmg += 5; return;
    case 29: this->to_hit += 2; this->to_dmg += 5; return;
    case 30: this->to_hit += 2; this->to_dmg += 5; return;
    case 31: this->to_hit += 3; this->to_dmg += 6; return;
  }

}
