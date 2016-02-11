#pragma once

#include <fstream>
#include <vector>
#include <list>
#include <string>

#include "daemons.h"

namespace Disk {
  using tag_type = unsigned long long;

  void save_tag(tag_type tag, std::ofstream& data);
  bool load_tag(tag_type tag, std::ifstream& data);

  // Simple types
  void save(tag_type tag, int&, std::ofstream& data);
  bool load(tag_type tag, int&, std::ifstream& data);

  void save(tag_type tag, std::string*, std::ofstream& data);
  bool load(tag_type tag, std::string**, std::ifstream& data);

  // Vector types
  void save(tag_type tag, std::vector<bool>*, std::ofstream& data);
  bool load(tag_type tag, std::vector<bool>**, std::ifstream& data);

  void save(tag_type tag, std::vector<std::string>*, std::ofstream& data);
  bool load(tag_type tag, std::vector<std::string>**, std::ifstream& data);

  // List types
  void save(tag_type tag, std::list<Daemons::Daemon>*, std::ofstream& data);
  bool load(tag_type tag, std::list<Daemons::Daemon>**, std::ifstream& data);

  void save(tag_type tag, std::list<Daemons::Fuse>*, std::ofstream& data);
  bool load(tag_type tag, std::list<Daemons::Fuse>**, std::ifstream& data);
}
