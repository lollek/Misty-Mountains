#pragma once

#include <vector>

#include "item.h"

class Scroll : public Item {
public:
  enum Type
  {
    CONFUSE   = 0,
    MAP       = 1,
    HOLD      = 2,
    SLEEP     = 3,
    ENCHARMOR = 4,
    ID        = 5,
    SCARE     = 6,
    FDET      = 7,
    TELEP     = 8,
    ENCH      = 9,
    CREATE    = 10,
    REMOVE    = 11,
    AGGR      = 12,
    PROTECT   = 13,
    NSCROLLS
  };

  ~Scroll();
  explicit Scroll();
  explicit Scroll(Type);
  explicit Scroll(Scroll const&) = default;

  Scroll* clone() const override;
  Scroll& operator=(Scroll const&) = default;
  Scroll& operator=(Scroll&&) = default;

  // Getters
  Type        get_type() const;
  std::string get_description() const override;
  bool        is_magic() const override;

  // Misc
  void read() const;

  // Static
  static std::string  name(Type subtype);
  static int          probability(Type subtype);
  static int          worth(Type subtype);
  static std::string& guess(Type subtype);
  static bool         is_known(Type subtype);
  static void         set_known(Type subtype);

  static void         init_scrolls();

private:
  Type subtype;

  static std::vector<std::string>  guesses;
  static std::vector<bool>         knowledge;
  static std::vector<std::string>  fake_name;
};

