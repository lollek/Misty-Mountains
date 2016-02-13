#pragma once

#include <string>

#include "monster.h"
#include "item.h"


class Armor : public Item {
public:
  enum Type {
    Robe,
    Softleatherarmor,
    Softstuddedleather,
    Hardleatherarmor,
    Hardstuddedleather,
    Softleatherringmail,
    Hardleatherringmail,
    Chainmail,
    Scalemail,
    Brigandinearmor,
    Laminatedarmor,
    Lamellararmor,
    Mithrilchainmail,

    NARMORS
  };

  ~Armor();
  explicit Armor(Type type, bool random_stats); // Armor of given type
  explicit Armor(bool random_stats);            // Armor of random type
  explicit Armor(std::ifstream&);
  explicit Armor(Armor const&) = default;

  Armor* clone() const override;
  Armor& operator=(Armor const&) = default;
  Armor& operator=(Armor&&) = default;

  // Setters
  void set_identified() override;
  void set_rustproof();

  // Getters
  std::string get_description() const override;
  bool        is_magic() const override;
  bool        is_identified() const override;
  bool        is_rustproof() const;

  void save(std::ofstream&) const override;
  bool load(std::ifstream&) override;

  // Static
  static std::string name(Type type);
  static int value(Type type);
  static int ac(Type type);

private:
  Type subtype;
  bool identified;
  bool rustproof;

  static unsigned long long constexpr TAG_ARMOR     = 0xc000000000000000ULL;
};

