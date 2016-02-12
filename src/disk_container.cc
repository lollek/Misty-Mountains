#include "disk.h"

using namespace std;

template <>
bool Disk::load<vector, bool>(tag_type tag, vector<bool>& container, std::ifstream& data) {
  if (!load_tag(tag, data)) { return false; }

  size_t size;
  if (!load(tag, size, data)) { return false; }

  for (size_t i = 0; i < size; ++i) {
    bool new_element;
    if (!load(tag, new_element, data)) { return false; }
    container.push_back(new_element);
  }
  return true;
}

