#pragma once

#include <vector>
#include <string>

#include "item.h"

class Weapon : public Item {
public:
  enum AmmoType {
    SlingShot,
    BowArrow,
    None
  };

  enum Type {
    Sling,
    Arrow,
    Rock,
    Dagger,
    Club,
    Quarterstaff,
    Shortbow,
    Throwingknife,
    Mace,
    Spear,
    Rapier,
    Kukri,
    Handaxe,
    Shortsword,
    Longsword,
    Wakizashi,
    Longbow,
    Throwingaxe,
    Morningstar,
    Battleaxe,
    Warhammer,
    Yari,
    Bastardsword,
    Halberd,
    Katana,
    Claymore,
    Nodachi,
    Warpike,
    Compositebow,

    NWEAPONS,
    NO_WEAPON
  };

  ~Weapon();
  explicit Weapon(Type subtype, bool random_stats);
  explicit Weapon(bool random_stats);
  explicit Weapon(std::ifstream&);
  explicit Weapon(Weapon const&) = default;

  Weapon* clone() const override;
  Weapon& operator=(Weapon const&) = default;
  Weapon& operator=(Weapon&&) = default;

  // Setters
  void set_identified() override;

  // Getters
  std::string get_description() const override;
  int         get_value() const override;
  int         get_base_value() const override;
  bool        is_magic() const override;
  bool        is_identified() const override;
  bool        is_stackable() const override;
  bool        autopickup() const override;
  bool        is_missile_launcher() const;
  AmmoType    get_ammo_used() const;
  AmmoType    get_ammo_type() const;
  int         get_ammo_multiplier() const;

  void save(std::ofstream&) const override;
  bool load(std::ifstream&) override;

  // Static
  static std::string name(Type type);
  static int worth(Type type);


private:
  Type       subtype;
  AmmoType   is_ammo_type;
  AmmoType   uses_ammo_type;
  int        ammo_multiplier;
  bool       identified;
  bool       good_missile;

  static unsigned long long constexpr TAG_WEAPON    = 0xb000000000000000ULL;
};

/* Drop an item someplace around here. */
void weapon_missile_fall(Item* obj, bool pr);
