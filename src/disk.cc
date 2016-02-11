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

void Disk::save(tag_type tag, int& element, ofstream& data) {
  save_tag(tag, data);
  data.write(reinterpret_cast<char*>(&element), sizeof(element));
}
bool Disk::load(tag_type tag, int& element, ifstream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }

  data.read(reinterpret_cast<char*>(&element), sizeof(element));
  return true;
}

void Disk::save(tag_type tag, string* element, ofstream& data) {
  save_tag(tag, data);

  size_t element_size = element->size();
  data.write(reinterpret_cast<char*>(&element_size), sizeof(element_size));
  data.write(element->c_str(), static_cast<long>(element_size));
}
bool Disk::load(tag_type tag, string** element, ifstream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }

  size_t element_size;
  data.read(reinterpret_cast<char*>(&element_size), sizeof(element_size));
  *element = new string;
  (*element)->resize(element_size);
  data.read(const_cast<char*>((*element)->c_str()), static_cast<long>(element_size));
  return true;
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
bool Disk::load(tag_type tag, vector<bool>** list, ifstream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }

  size_t size;
  data.read(reinterpret_cast<char*>(&size), sizeof(size));

  *list = new vector<bool>(size);
  for (size_t i = 0; i < size; ++i) {
    bool new_value;
    data.read(reinterpret_cast<char*>(&new_value), sizeof(new_value));
    (*list)->at(i) = new_value;
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
bool Disk::load(tag_type tag, vector<string>** list, ifstream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }

  size_t size;
  data.read(reinterpret_cast<char*>(&size), sizeof(size));

  *list = new vector<string>(size);
  for (size_t i = 0; i < size; ++i) {
    size_t element_size;
    data.read(reinterpret_cast<char*>(&element_size), sizeof(element_size));
    string element;
    element.resize(element_size);
    data.read(const_cast<char*>(element.c_str()), static_cast<long>(element_size));
    (*list)->at(i) = element;
  }
  return true;
}

void Disk::save(tag_type tag, list<Daemons::Daemon>* container, ofstream& data) {
  save_tag(tag, data);

  size_t size = container->size();
  data.write(reinterpret_cast<char*>(&size), sizeof(size));

  for (Daemons::Daemon& daemon : *container) {
    data.write(reinterpret_cast<char*>(&daemon.type), sizeof(daemon.type));
    data.write(reinterpret_cast<char*>(&daemon.func), sizeof(daemon.func));
  }
}
bool Disk::load(tag_type tag, list<Daemons::Daemon>** container, ifstream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }

  size_t size;
  data.read(reinterpret_cast<char*>(&size), sizeof(size));

  *container = new list<Daemons::Daemon>(size);
  for (size_t i = 0; i < size; ++i) {
    Daemons::Daemon daemon;
    data.read(reinterpret_cast<char*>(&daemon.type), sizeof(daemon.type));
    data.read(reinterpret_cast<char*>(&daemon.func), sizeof(daemon.func));
    (*container)->push_back(daemon);
  }
  return true;
}

void Disk::save(tag_type tag, list<Daemons::Fuse>* container, ofstream& data) {
  save_tag(tag, data);

  size_t size = container->size();
  data.write(reinterpret_cast<char*>(&size), sizeof(size));

  for (Daemons::Fuse& fuse : *container) {
    data.write(reinterpret_cast<char*>(&fuse.type), sizeof(fuse.type));
    data.write(reinterpret_cast<char*>(&fuse.func), sizeof(fuse.func));
    data.write(reinterpret_cast<char*>(&fuse.time), sizeof(fuse.time));
  }
}
bool Disk::load(tag_type tag, list<Daemons::Fuse>** container, ifstream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }

  size_t size;
  data.read(reinterpret_cast<char*>(&size), sizeof(size));

  *container = new list<Daemons::Fuse>(size);
  for (size_t i = 0; i < size; ++i) {
    Daemons::Fuse fuse;
    data.read(reinterpret_cast<char*>(&fuse.type), sizeof(fuse.type));
    data.read(reinterpret_cast<char*>(&fuse.func), sizeof(fuse.func));
    data.read(reinterpret_cast<char*>(&fuse.time), sizeof(fuse.time));
    (*container)->push_back(fuse);
  }
  return true;
}
