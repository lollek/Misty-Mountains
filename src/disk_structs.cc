#include "disk.h"

//Daemons::Daemon
template<>
void Disk::save<Daemons::Daemon>(tag_type tag, Daemons::Daemon const& element, std::ofstream& data) {
  save_tag(tag, data);

  data.write(reinterpret_cast<char const*>(&element.type), sizeof(element.type));
  data.write(reinterpret_cast<char const*>(&element.func), sizeof(element.func));
}
template<>
bool Disk::load<Daemons::Daemon>(tag_type tag, Daemons::Daemon& element, std::ifstream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }

  data.read(reinterpret_cast<char*>(&element.type), sizeof(element.type));
  data.read(reinterpret_cast<char*>(&element.func), sizeof(element.func));
  return true;
}

//Daemons::Fuse
template<>
void Disk::save<Daemons::Fuse>(tag_type tag, Daemons::Fuse const& element, std::ofstream& data) {
  save_tag(tag, data);

  data.write(reinterpret_cast<char const*>(&element.type), sizeof(element.type));
  data.write(reinterpret_cast<char const*>(&element.func), sizeof(element.func));
  data.write(reinterpret_cast<char const*>(&element.time), sizeof(element.time));
}
template<>
bool Disk::load<Daemons::Fuse>(tag_type tag, Daemons::Fuse& element, std::ifstream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }

  data.read(reinterpret_cast<char*>(&element.type), sizeof(element.type));
  data.read(reinterpret_cast<char*>(&element.func), sizeof(element.func));
  data.read(reinterpret_cast<char*>(&element.time), sizeof(element.time));
  return true;
}

// Coordinate
template <>
void save<Coordinate>(tag_type tag, Coordinate const& element, std::ofstream& data) {
  save_tag(tag, data);
  data.write(reinterpret_cast<char const*>(&element.x), sizeof(element.x));
  data.write(reinterpret_cast<char const*>(&element.y), sizeof(element.y));
}
template <>
bool load<Coordinate>(tag_type tag, Coordinate& element, std::ifstream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }
  data.read(const_cast<char*>(element.x), static_cast<long>(element.x));
  data.read(const_cast<char*>(element.y), static_cast<long>(element.y));
}
