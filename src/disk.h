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
  template <class T>
  void save(tag_type, T const&, std::ofstream&);
  template <class T>
  bool load(tag_type, T&, std::ifstream&);

  // Pointers to simple types
  template <class T>
  void save(tag_type, T*, std::ofstream&);
  template <class T>
  bool load(tag_type, T**, std::ifstream&);

  // Containers of simple types
  template <template <class, class> class C, class T>
  void save(tag_type, C<T, std::allocator<T>> const&, std::ofstream&);
  template <template <class, class> class C, class T>
  bool load(tag_type, C<T, std::allocator<T>>&, std::ifstream&);

#include "disk_simple.m"
#include "disk_pointers.m"
#include "disk_structs.m"
#include "disk_container.m"

}
