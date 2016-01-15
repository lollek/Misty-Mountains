#pragma once

#include <vector>

#include "things.h"

class MonsterList {
public:
  using ElementType = monster*;
  using ConstElementType = monster const*;
  using ListType = vector<ElementType>;

  void attach(ElementType);
  void detach(ElementType);
  bool contains(ConstElementType) const;

  operator ListType&();

private:
  ListType list;
};
