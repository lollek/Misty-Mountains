#pragma once

#include <vector>
#include <string>

#include "Character.h"


class Potion : public Item {
public:
  enum Type {
    CONFUSION  = 0,
    LSD        = 1,
    POISON     = 2,
    STRENGTH   = 3,
    SEEINVIS   = 4,
    HEALING    = 5,
    MFIND      = 6,
    TFIND      = 7,
    RAISE      = 8,
    XHEAL      = 9,
    HASTE      = 10,
    RESTORE    = 11,
    BLIND      = 12,
    LEVIT      = 13,
    NPOTIONS
  };

  ~Potion();
  explicit Potion();     // Random potion
  explicit Potion(Type); // Potion of given type
  explicit Potion(Potion const&) = default;

  Potion* clone() const override;
  Potion& operator=(Potion const&) = default;
  Potion& operator=(Potion&&) = default;

  // Getters
  Type        get_type() const;
  std::string get_description() const override;
  bool        is_magic() const override;

  // Misc
  void quaffed_by(Character&); // Someone drank the potion

  // Static
  static std::string  name(Type subtype);
  static int          probability(Type subtype);
  static int          worth(Type subtype);
  static std::string& guess(Type subtype);
  static bool         is_known(Type subtype);
  static void         set_known(Type subtype);

  static void         init_potions();
  static void         free_potions();

private:
  Type subtype;

  static std::vector<std::string>*        guesses;
  static std::vector<bool>*               knowledge;
  static std::vector<std::string const*>* colors;
};

bool potion_quaff_something(void);  /* Quaff a potion from the pack */
