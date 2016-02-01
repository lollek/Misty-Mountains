#pragma once

#include <vector>
#include <string>

#include "item.h"

class Wand : public Item {
public:
  enum Type {
    LIGHT     = 0,
    INVIS     = 1,
    ELECT     = 2,
    FIRE      = 3,
    COLD      = 4,
    POLYMORPH = 5,
    MISSILE   = 6,
    HASTE_M   = 7,
    SLOW_M    = 8,
    DRAIN     = 9,
    NOP       = 10,
    TELAWAY   = 11,
    TELTO     = 12,
    CANCEL    = 13,
    NWANDS
  };

  ~Wand();
  Wand();     // Random wand
  explicit Wand(Type); // Wand of given type
  explicit Wand(Wand const&) = default;

  Wand* clone() const override;
  Wand& operator=(Wand const&) = default;
  Wand& operator=(Wand&&) = default;


  // Modifier
  void modify_charges(int amount);

  // Setters
  void set_identified();
  void set_charges(int amount);

  // Getters
  std::string get_description() const override;
  bool        is_magic() const override;
  std::string get_material() const;
  bool        is_identified() const;
  int         get_charges() const;


  // Static
  static void init_wands();

  static std::string        name(Type subtype);
  static int                probability(Type subtype);
  static int                worth(Type subtype);
  static std::string&       guess(Type subtype);
  static bool               is_known(Type subtype);
  static void               set_known(Type subtype);
  static std::string const& material(Type subtype);

private:
  int charges;
  bool identified;
  Type subtype;

  static std::vector<std::string> materials;
  static std::vector<std::string> guesses;
  static std::vector<bool>        known;
};

/* Perform a zap with a wand */
bool wand_zap();
