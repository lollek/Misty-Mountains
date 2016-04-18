#include "item.h"

#include "disk.h"

using namespace std;

// string
template <>
void Disk::save<string>(tag_type tag, string const& element, ostream& data) {
  save_tag(tag, data);
  size_t const element_size{element.size()};
  data.write(reinterpret_cast<char const*>(&element_size),
             sizeof(element_size));
  data.write(element.c_str(), static_cast<long>(element_size));
}
template <>
bool Disk::load<string>(tag_type tag, string& element, istream& data) {
  if (!load_tag(tag, data)) { return false; }
  size_t element_size;
  data.read(reinterpret_cast<char*>(&element_size), sizeof(element_size));
  element.resize(element_size);
  data.read(const_cast<char*>(element.c_str()),
            static_cast<long>(element_size));

  return true;
}

// Item
template <>
void Disk::save<Item>(tag_type tag, Item const& element, ostream& data) {
  save_tag(tag, data);
  element.save(data);
}
template <>
bool Disk::load<Item>(tag_type tag, Item& element, istream& data) {
  if (!load_tag(tag, data)) { return false; }
  element.load(data);
  return true;
}

// Feat
template <>
void Disk::save<Character::Feat>(tag_type tag, Character::Feat const& element,
                                 ostream& data) {
  save_tag(tag, data);
  data.write(reinterpret_cast<char const*>(&element), sizeof(element));
}
template <>
bool Disk::load<Character::Feat>(tag_type tag, Character::Feat& element,
                                 istream& data) {
  if (!load_tag(tag, data)) { return false; }
  data.read(reinterpret_cast<char*>(&element), sizeof(element));
  return true;
}
