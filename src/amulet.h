#pragma once

#include <string>

#include "item.h"

class Amulet : public Item {
public:
  ~Amulet();

  explicit Amulet();
  explicit Amulet(std::istream&);
  explicit Amulet(Amulet const&) = default;

  Amulet* clone() const override;
  Amulet& operator=(Amulet const&) = default;
  Amulet& operator=(Amulet&&) = default;

  // setters
  void        set_identified() override;

  // getters
  std::string get_description() const override;
  bool        is_magic() const override;
  bool        is_identified() const override;
  bool        is_stackable() const override;
  bool        autopickup() const override;
  int         get_value() const override;
  int         get_base_value() const override;

private:
};

