#include <vector>

#include "disk.h"

using namespace std;

template <>
void Disk::save<vector, bool>(tag_type tag, vector<bool> const& container,
                              std::ostream& data) {
  save_tag(tag, data);

  size_t const size{container.size()};
  save(tag, size, data);

  for (bool const element : container) {
    save(tag, element, data);
  }
}

template <>
bool Disk::load<vector, bool>(tag_type tag, vector<bool>& container,
                              std::istream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }

  size_t size;
  if (!load(tag, size, data)) {
    return false;
  }

  for (size_t i{0}; i < size; ++i) {
    bool new_element;
    if (!load(tag, new_element, data)) {
      return false;
    }
    container.push_back(new_element);
  }
  return true;
}
