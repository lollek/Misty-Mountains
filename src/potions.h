#pragma once

#include <vector>
#include <string>
#include <fstream>

#include "item.h"
#include "character.h"


class Potion : public Item {
public:
  enum Type : int {
    CONFUSION,
    POISON,
    STRENGTH,
    SEEINVIS,
    HEALING,
    MFIND,
    TFIND,
    RAISE,
    XHEAL,
    HASTE,
    RESTORE,
    BLIND,
    LEVIT,
    NPOTIONS
  };

  ~Potion();
  explicit Potion();     // Random potion
  explicit Potion(Type); // Potion of given type
  explicit Potion(Potion const&) = default;

  Potion* clone() const override;
  Potion& operator=(Potion const&) = default;
  Potion& operator=(Potion&&) = default;

  // Setters
  void        set_identified() override;

  // Getters
  Type        get_type() const;
  std::string get_description() const override;
  bool        is_magic() const override;
  bool        is_identified() const override;

  // Misc
  void quaffed_by(Character&); // Someone drank the potion

  virtual void save(std::ofstream&) const override;
  virtual bool load(std::ifstream&) override;

  // Static
  static std::string  name(Type subtype);
  static int          probability(Type subtype);
  static int          worth(Type subtype);
  static std::string& guess(Type subtype);
  static bool         is_known(Type subtype);
  static void         set_known(Type subtype);

  static void         init_potions();
  static void         save_potions(std::ofstream&);
  static void         load_potions(std::ifstream&);
  static void         free_potions();

private:
  Type subtype;

  static std::vector<std::string>* colors;
  static std::vector<bool>*        knowledge;
  static std::vector<std::string>* guesses;

  static unsigned long long constexpr TAG_POTION    = 0x2000000000000000ULL;
  static unsigned long long constexpr TAG_COLORS    = 0x2000000000000001ULL;
  static unsigned long long constexpr TAG_KNOWLEDGE = 0x2000000000000002ULL;
  static unsigned long long constexpr TAG_GUESSES   = 0x2000000000000003ULL;
};

bool potion_quaff_something(void);  /* Quaff a potion from the pack */
