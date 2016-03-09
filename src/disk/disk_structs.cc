#include "disk.h"

//Daemons::Daemon
static_assert(sizeof(Daemons::Daemon) ==
    sizeof(Daemons::Daemon::type) +
    sizeof(Daemons::Daemon::func),
    "Daemons::Daemon size has changed");
template<>
void Disk::save<Daemons::Daemon>(tag_type tag, Daemons::Daemon const& element, std::ostream& data) {
  save_tag(tag, data);

  data.write(reinterpret_cast<char const*>(&element.type), sizeof(element.type));
  data.write(reinterpret_cast<char const*>(&element.func), sizeof(element.func));
}
template<>
bool Disk::load<Daemons::Daemon>(tag_type tag, Daemons::Daemon& element, std::istream& data) {
  if (!load_tag(tag, data)) { return false; }

  data.read(reinterpret_cast<char*>(&element.type), sizeof(element.type));
  data.read(reinterpret_cast<char*>(&element.func), sizeof(element.func));
  return true;
}

//Daemons::Fuse
static_assert(sizeof(Daemons::Fuse) ==
    sizeof(Daemons::Fuse::type) +
    sizeof(Daemons::Fuse::func) +
    sizeof(Daemons::Fuse::func),
    "Daemons::Func size has changed");
template<>
void Disk::save<Daemons::Fuse>(tag_type tag, Daemons::Fuse const& element, std::ostream& data) {
  save_tag(tag, data);

  data.write(reinterpret_cast<char const*>(&element.type), sizeof(element.type));
  data.write(reinterpret_cast<char const*>(&element.func), sizeof(element.func));
  data.write(reinterpret_cast<char const*>(&element.time), sizeof(element.time));
}
template<>
bool Disk::load<Daemons::Fuse>(tag_type tag, Daemons::Fuse& element, std::istream& data) {
  if (!load_tag(tag, data)) { return false; }

  data.read(reinterpret_cast<char*>(&element.type), sizeof(element.type));
  data.read(reinterpret_cast<char*>(&element.func), sizeof(element.func));
  data.read(reinterpret_cast<char*>(&element.time), sizeof(element.time));
  return true;
}

// Coordinate
static_assert(sizeof(Coordinate) ==
    sizeof(Coordinate::x) +
    sizeof(Coordinate::y),
    "Coordinate size has changed");
template <>
void Disk::save<Coordinate>(tag_type tag, Coordinate const& element, std::ostream& data) {
  save_tag(tag, data);
  data.write(reinterpret_cast<char const*>(&element.x), sizeof(element.x));
  data.write(reinterpret_cast<char const*>(&element.y), sizeof(element.y));
}
template <>
bool Disk::load<Coordinate>(tag_type tag, Coordinate& element, std::istream& data) {
  if (!load_tag(tag, data)) { return false; }
  data.read(reinterpret_cast<char*>(&element.x), sizeof(element.x));
  data.read(reinterpret_cast<char*>(&element.y), sizeof(element.y));
  return true;
}

// damage
static_assert(sizeof(damage) ==
    sizeof(damage::sides) +
    sizeof(damage::dices),
    "damage size has changed");
template <>
void Disk::save<damage>(tag_type tag, damage const& element, std::ostream& data) {
  save_tag(tag, data);
  data.write(reinterpret_cast<char const*>(&element.sides), sizeof(element.sides));
  data.write(reinterpret_cast<char const*>(&element.dices), sizeof(element.dices));
}
template <>
bool Disk::load<damage>(tag_type tag, damage& element, std::istream& data) {
  if (!load_tag(tag, data)) { return false; }
  data.read(reinterpret_cast<char*>(&element.sides), sizeof(element.sides));
  data.read(reinterpret_cast<char*>(&element.dices), sizeof(element.dices));
  return true;
}

