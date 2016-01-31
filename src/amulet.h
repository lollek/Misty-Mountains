#pragma once

#include <string>

#include "item.h"

class Amulet : public Item {
public:
  ~Amulet();

  explicit Amulet();
  explicit Amulet(Amulet const&) = default;

  Amulet* clone() const override;
  Amulet& operator=(Amulet const&) = default;
  Amulet& operator=(Amulet&&) = default;

  // getters
  std::string get_description() const override;

private:
};

std::string amulet_description(Item const* item);

