#pragma once

#include <vector>
#include <list>

#include "item.h"

class Shop {
public:
  ~Shop();

  explicit Shop();
  explicit Shop(Shop const&) = default;

  Shop& operator=(Shop const&) = default;
  Shop& operator=(Shop &&) = default;

  void enter();

private:
  void print() const;
  int sell();

  int  buy_value(Item const*) const;
  int  sell_value(Item const*) const;

  std::vector<Item*> inventory;
  std::list<Item*> limited_inventory;

  static int constexpr max_items_per_page = 25;
};
