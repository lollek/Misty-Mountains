#include "item.h"

#include "disk.h"

// std::string
template <>
void Disk::save<std::string>(tag_type tag, std::string const& element, std::ofstream& data) {
  save_tag(tag, data);
  size_t element_size = element.size();
  data.write(reinterpret_cast<char const*>(&element_size), sizeof(element_size));
  data.write(element.c_str(), static_cast<long>(element_size));
}
template <>
bool Disk::load<std::string>(tag_type tag, std::string& element, std::ifstream& data) {
  if (!load_tag(tag, data)) { return false; }
  size_t element_size;
  data.read(reinterpret_cast<char*>(&element_size), sizeof(element_size));
  element.resize(element_size);
  data.read(const_cast<char*>(element.c_str()), static_cast<long>(element_size));

  return true;
}

// item
template <>
void Disk::save<Item>(tag_type tag, Item const& element, std::ofstream& data) {
  save_tag(tag, data);
  element.save(data);
}
template <>
bool Disk::load<Item>(tag_type tag, Item& element, std::ifstream& data) {
  if (!load_tag(tag, data)) { return false; }
  element.load(data);
  return true;
}
