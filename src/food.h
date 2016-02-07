#pragma once

#include <string>

#include "item.h"

class Food : public Item {
public:
  enum Type {
    Fruit,
    IronRation,
    NFOODS
  };

  ~Food();

  explicit Food();
  explicit Food(Type subtype);
  explicit Food(Food const&) = default;

  Food* clone() const override;
  Food& operator=(Food const&) = default;
  Food& operator=(Food&&) = default;

  // Getters
  Type        get_type() const;
  std::string get_description() const override;
  bool        is_magic() const override;

private:
  Type subtype;
};

