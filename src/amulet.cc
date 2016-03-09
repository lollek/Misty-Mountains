#include <string>

#include "error_handling.h"
#include "io.h"

#include "item/amulet.h"

using namespace std;

Amulet::~Amulet() {}

Amulet::Amulet() : Item() {
  set_attack_damage({1, 2});
  set_throw_damage({1, 2});
  o_type    = IO::Amulet;
}

Amulet::Amulet(std::istream& data) {
  load(data);
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

bool Amulet::is_identified() const {
  return true;
}

void Amulet::set_identified() {
}

int Amulet::get_base_value() const {
  return 1000;
}

int Amulet::get_value() const {
  return get_base_value();
}

bool Amulet::is_stackable() const {
  return false;
}

bool Amulet::autopickup() const {
  return true;
}
