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
  explicit Armor(Armor const&) = default;

  Armor* clone() const override;
  Armor& operator=(Armor const&) = default;
  Armor& operator=(Armor&&) = default;

  // Setters
  void set_identified();
  void set_not_identified();

  // Getters
  bool        is_identified() const;
  std::string get_description() const override;
  bool        is_magic() const override;

  // Static
  static int probability(Type type);
  static std::string name(Type type);
  static int value(Type type);
  static int ac(Type type);

private:
  Type subtype;
  bool identified;
};

std::string armor_description(Item const* item);
