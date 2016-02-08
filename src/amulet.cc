#include <string>

#include "error_handling.h"
#include "io.h"

#include "amulet.h"

using namespace std;

Amulet::~Amulet() {}

Amulet::Amulet() : Item() {
  set_attack_damage({1, 2});
  set_throw_damage({1, 2});
  o_type    = IO::Amulet;
}

string Amulet::get_description() const {
  return "the Amulet of Yendor";
}

Amulet* Amulet::clone() const {
  return new Amulet(*this);
}

bool Amulet::is_magic() const {
  return true;
}

