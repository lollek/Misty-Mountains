#pragma once

#include <vector>
#include <string>

#include "item.h"

class Ring : public Item {
public:
  enum Type {
    PROTECT = 0,
    ADDSTR = 1,
    SUSTSTR = 2,
    SEARCH = 3,
    SEEINVIS = 4,
    NOP = 5,
    AGGR = 6,
    ADDHIT = 7,
    ADDDAM = 8,
    REGEN = 9,
    DIGEST = 10,
    TELEPORT = 11,
    STEALTH = 12,
    SUSTARM = 13,
    NRINGS
  };

  ~Ring();
  explicit Ring(Type type, bool random_stats);
  explicit Ring(bool random_stats);
  explicit Ring(Ring const&) = default;

  Ring* clone() const override;
  Ring& operator=(Ring const&) = default;
  Ring& operator=(Ring&&) = default;

  // Getters
  std::string get_description() const override;
  bool        is_magic() const override;

  // Static
  static int          probability(Type type);
  static std::string  name(Type type);
  static int          worth(Type type);
  static std::string& guess(Type type);
  static bool         is_known(Type type);
  static void         set_known(Type type);

  static void         init_rings();

private:
  Type subtype;

  static std::vector<std::string> materials;
  static std::vector<std::string> guesses;
  static std::vector<bool>        known;
};

std::string ring_description(Item const* item);
