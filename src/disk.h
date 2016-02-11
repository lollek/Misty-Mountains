#pragma once

#include <fstream>
#include <vector>
#include <string>

namespace Disk {
  using tag_type = unsigned long long;

  void save_tag(tag_type tag, std::ofstream& data);
  bool load_tag(tag_type tag, std::ifstream& data);

  void save(tag_type tag, std::vector<bool>*, std::ofstream& data);
  bool load(tag_type tag, std::vector<bool>*, std::ifstream& data);

  void save(tag_type tag, std::vector<std::string>*, std::ofstream& data);
  bool load(tag_type tag, std::vector<std::string>*, std::ifstream& data);
}
