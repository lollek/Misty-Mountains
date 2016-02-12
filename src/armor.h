#pragma once

#include <string>

#include "monster.h"
#include "item.h"


class Armor : public Item {
public:
  enum Type {
    LEATHER = 0,
    RING_MAIL = 1,
    STUDDED_LEATHER = 2,
    SCALE_MAIL = 3,
    CHAIN_MAIL = 4,
    SPLINT_MAIL = 5,
    BANDED_MAIL = 6,
    PLATE_MAIL = 7,
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

  // Getters
  std::string get_description() const override;
  bool        is_magic() const override;
  bool        is_identified() const override;

  void save(std::ofstream&) const override;
  bool load(std::ifstream&) override;

  // Static
  static int probability(Type type);
  static std::string name(Type type);
  static int value(Type type);
  static int ac(Type type);

private:
  Type subtype;
  bool identified;

  static unsigned long long constexpr TAG_ARMOR     = 0xc000000000000000ULL;
};

