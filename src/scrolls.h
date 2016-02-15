#pragma once

#include <vector>
#include <fstream>

#include "item.h"

class Scroll : public Item {
public:
  enum Type {
    CONFUSE,
    MAP,
    HOLD,
    SLEEP,
    ENCHARMOR,
    ID,
    SCARE,
    FDET,
    TELEP,
    ENCH,
    CREATE,
    REMOVE,
    AGGR,
    PROTECT,
    NSCROLLS
  };

  ~Scroll();
  explicit Scroll();
  explicit Scroll(Type);
  explicit Scroll(std::ifstream&);
  explicit Scroll(Scroll const&) = default;

  Scroll* clone() const override;
  Scroll& operator=(Scroll const&) = default;
  Scroll& operator=(Scroll&&) = default;

  // Setters
  void       set_identified() override;

  // Getters
  Type        get_type() const;
  std::string get_description() const override;
  bool        is_magic() const override;
  bool        is_identified() const override;
  bool        is_stackable() const override;
  bool        autopickup() const override;
  int         get_value() const override;
  int         get_base_value() const override;

  // Misc
  void read() const;

  virtual void save(std::ofstream&) const override;
  virtual bool load(std::ifstream&) override;

  // Static
  static std::string  name(Type subtype);
  static int          worth(Type subtype);
  static std::string& guess(Type subtype);
  static bool         is_known(Type subtype);
  static void         set_known(Type subtype);

  static void         init_scrolls();
  static void         save_scrolls(std::ofstream&);
  static void         load_scrolls(std::ifstream&);
  static void         free_scrolls();

private:
  Type subtype;

  static std::vector<std::string>* fake_name;
  static std::vector<bool>*        knowledge;
  static std::vector<std::string>* guesses;

  static unsigned long long constexpr TAG_SCROLL    = 0x1000000000000000ULL;
  static unsigned long long constexpr TAG_FAKE_NAME = 0x1000000000000001ULL;
  static unsigned long long constexpr TAG_KNOWLEDGE = 0x1000000000000002ULL;
  static unsigned long long constexpr TAG_GUESSES   = 0x1000000000000003ULL;
};

