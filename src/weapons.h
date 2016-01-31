#pragma once

#include <vector>
#include <string>

#include "item.h"

class Weapon : public Item {
public:
  enum Type {
    MACE     = 0,
    SWORD    = 1,
    BOW      = 2,
    ARROW    = 3,
    DAGGER   = 4,
    TWOSWORD = 5,
    DART     = 6,
    SHIRAKEN = 7,
    SPEAR    = 8,
    NWEAPONS,
    NO_WEAPON
  };

  ~Weapon();
  explicit Weapon(Type subtype, bool random_stats);
  explicit Weapon(bool random_stats);
  explicit Weapon(Weapon const&) = default;

  Weapon& operator=(Weapon const&) = default;
  Weapon& operator=(Weapon&&) = default;

  // Setters
  void set_identified();
  void set_not_identified();

  // Getters
  bool is_identified() const;
  std::string get_description() const;

  // Static
  static int probability(Type type);
  static std::string name(Type type);
  static int worth(Type type);


private:
  Type subtype;
  bool identified;
};

/* Drop an item someplace around here. */
void weapon_missile_fall(Item* obj, bool pr);

std::string weapon_description(Item const* item);
