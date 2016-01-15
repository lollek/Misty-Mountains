#include <vector>
#include <exception>

using namespace std;

#include "things.h"

#include "MonsterList.h"

void MonsterList::attach(ElementType element) {
  this->list.push_back(element);
}

void MonsterList::detach(ElementType element) {
  auto position = find(begin(this->list), end(this->list), element);
  if (position == end(this->list)) {
    throw invalid_argument("Element was not in list");
  }
  this->list.erase(position);
}

bool MonsterList::contains(ConstElementType element) const {
  return find(begin(this->list), end(this->list), element) != end(this->list);
}

MonsterList::operator ListType&() {
  return this->list;
}
