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

