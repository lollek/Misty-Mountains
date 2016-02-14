#pragma once

#include <vector>
#include <string>
#include <fstream>

#include "item.h"

class Ring : public Item {
public:
  enum Type {
    Adornment,
    AggravateMonsters,
    Teleportation,
    Protection,
    Searching,
    SlowDigestation,
    Damage,
    Accuracy,
    Regeneration,
    Stealth,
    Strength,
    SeeInvisible,
    SustainStrenght,
    Speed,

    NRINGS
  };

  ~Ring();
  explicit Ring(Type type);
  explicit Ring();
  explicit Ring(std::ifstream&);
  explicit Ring(Ring const&) = default;

  Ring* clone() const override;
  Ring& operator=(Ring const&) = default;
  Ring& operator=(Ring&&) = default;

  // Setters
  void        set_identified() override;

  // Getters
  std::string get_description() const override;
  bool        is_magic() const override;
  bool        is_identified() const override;
  int         get_value() const override;
  int         get_base_value() const override;
  bool        is_stackable() const override;

  void save(std::ofstream&) const override;
  bool load(std::ifstream&) override;

  // Static
  static std::string  name(Type type);
  static int          worth(Type type);
  static std::string& guess(Type type);
  static bool         is_known(Type type);
  static void         set_known(Type type);

  static void         init_rings();
  static void         save_rings(std::ofstream&);
  static void         load_rings(std::ifstream&);
  static void         free_rings();

private:
  Type subtype;
  bool identified;

  static std::vector<std::string>* materials;
  static std::vector<std::string>* guesses;
  static std::vector<bool>*        known;

  static unsigned long long constexpr TAG_RINGS     = 0x3000000000000000ULL;
  static unsigned long long constexpr TAG_MATERIALS = 0x3000000000000001ULL;
  static unsigned long long constexpr TAG_KNOWN     = 0x3000000000000002ULL;
  static unsigned long long constexpr TAG_GUESSES   = 0x3000000000000003ULL;
};

