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
  void sell();

  std::vector<Item*> inventory;
  std::list<Item*> limited_inventory;
};
