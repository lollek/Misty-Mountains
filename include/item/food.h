#pragma once

#include <istream>
#include <ostream>
#include <string>

#include "item.h"

class Food : public Item {
 public:
  enum Type { Fruit, IronRation, NFOODS };

  ~Food();

  explicit Food();
  explicit Food(Type subtype);
  explicit Food(std::istream&);
  explicit Food(Food const&) = default;

  Food* clone() const override;
  Food& operator=(Food const&) = default;
  Food& operator=(Food&&) = default;

  virtual void save(std::ostream&) const override;
  virtual bool load(std::istream&) override;

  // Setters
  void set_identified() override;

  // Getters
  Type get_type() const;
  std::string get_description() const override;
  bool is_magic() const override;
  bool is_identified() const override;
  bool is_stackable() const override;
  bool autopickup() const override;
  int get_value() const override;
  int get_base_value() const override;
  int get_nutrition_value() const;

  // Statics
  static int food_for_type(Type subtype);

 private:
  Type subtype;
  int food_value;

  static unsigned long long constexpr TAG_FOOD = 0xa000000000000000ULL;
};
