#include "damage.h"

bool damage::operator==(damage const& other) const {
  return dices == other.dices && sides == other.sides;
}

bool damage::operator!=(damage const& other) const { return !(*this == other); }
