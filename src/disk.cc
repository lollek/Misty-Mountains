#include "disk.h"

using namespace std;

void Disk::save_tag(tag_type tag, std::ofstream& data) {
  data.write(reinterpret_cast<char*>(&tag), sizeof(tag));
}
bool Disk::load_tag(tag_type tag, std::ifstream& data) {
  tag_type loaded_tag;
  data.read(reinterpret_cast<char*>(&loaded_tag), sizeof(loaded_tag));
  return loaded_tag == tag;
}

void Disk::save(tag_type tag, vector<bool>* list, ofstream& data) {
  save_tag(tag, data);

  size_t size = list->size();
  data.write(reinterpret_cast<char*>(&size), sizeof(size));

  for (size_t i = 0; i < list->size(); ++i) {
    bool element = list->at(i);
    data.write(reinterpret_cast<char*>(&element), sizeof(element));
  }
}
bool Disk::load(tag_type tag, vector<bool>* list, ifstream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }

  size_t size;
  data.read(reinterpret_cast<char*>(&size), sizeof(size));

  list = new vector<bool>(size);
  for (size_t i = 0; i < size; ++i) {
    bool new_value;
    data.read(reinterpret_cast<char*>(&new_value), sizeof(new_value));
    list->at(i) = new_value;
  }
  return true;
}

void Disk::save(tag_type tag, vector<string>* list, ofstream& data) {
  save_tag(tag, data);

  size_t size = list->size();
  data.write(reinterpret_cast<char*>(&size), sizeof(size));

  for (size_t i = 0; i < list->size(); ++i) {
    string element = list->at(i);
    size_t element_size = element.size();
    data.write(reinterpret_cast<char*>(&element_size), sizeof(element_size));
    data.write(element.c_str(), static_cast<long>(element_size));
  }
}
bool Disk::load(tag_type tag, vector<string>* list, ifstream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }

  size_t size;
  data.read(reinterpret_cast<char*>(&size), sizeof(size));

  list = new vector<string>(size);
  for (size_t i = 0; i < size; ++i) {
    size_t element_size;
    data.read(reinterpret_cast<char*>(&element_size), sizeof(element_size));
    string element;
    element.resize(element_size);
    data.read(const_cast<char*>(element.c_str()), static_cast<long>(element_size));
    list->at(i) = element;
  }
  return true;
}
