#pragma once

#include <string>
#include <fstream>

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
  explicit Food(std::ifstream&);
  explicit Food(Food const&) = default;

  Food* clone() const override;
  Food& operator=(Food const&) = default;
  Food& operator=(Food&&) = default;

  virtual void save(std::ofstream&) const override;
  virtual bool load(std::ifstream&) override;

  // Setters
  void        set_identified() override;

  // Getters
  Type        get_type() const;
  std::string get_description() const override;
  bool        is_magic() const override;
  bool        is_identified() const override;
  int         get_nutrition_value() const;

  // Statics
  static int  food_for_type(Type subtype);

private:
  Type subtype;
  int  food_value;


  static unsigned long long constexpr TAG_FOOD      = 0xa000000000000000ULL;
};

