#pragma once

#include <string>

#include "item.h"

class Gold : public Item {
public:
  ~Gold();

  explicit Gold();
  explicit Gold(int amount);
  explicit Gold(Gold const&) = default;

  Gold* clone() const override;
  Gold& operator=(Gold const&) = default;
  Gold& operator=(Gold&&) = default;

  // getters
  int         get_amount() const;
  std::string get_description() const override;
  bool        is_magic() const override;

  // static
  static int random_gold_amount();

private:
  int amount;
};

std::string gold_description(Item const* item);
