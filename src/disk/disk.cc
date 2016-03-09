#include "disk.h"

using namespace std;

void Disk::save_tag(tag_type tag, ostream& data) {
  data.write(reinterpret_cast<char*>(&tag), sizeof(tag));
}

bool Disk::load_tag(tag_type tag, istream& data) {
  tag_type loaded_tag;
  data.read(reinterpret_cast<char*>(&loaded_tag), sizeof(loaded_tag));
  return loaded_tag == tag;
}

