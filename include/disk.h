#pragma once

#include <istream>
#include <ostream>
#include <vector>
#include <list>
#include <string>

#include "daemons.h"     // disk/structs.m
#include "coordinate.h"  // disk/structs.m
#include "damage.h"      // disk/structs.m
#include "item.h"        // disk/simple_types.m
#include "character.h"   // disk/simple_types.m

namespace Disk {
  using tag_type = unsigned long long;

  void save_tag(tag_type tag, std::ostream& data);
  bool load_tag(tag_type tag, std::istream& data);

  // Simple types
  template <class T>
  void save(tag_type, T const&, std::ostream&);
  template <class T>
  bool load(tag_type, T&, std::istream&);

  // Pointers to simple types
  template <class T>
  void save(tag_type, T*, std::ostream&);
  template <class T>
  bool load(tag_type, T*&, std::istream&);

  // Containers of simple types
  template <template <class, class> class C, class T>
  void save(tag_type, C<T, std::allocator<T>> const&, std::ostream&);
  template <template <class, class> class C, class T>
  bool load(tag_type, C<T, std::allocator<T>>&, std::istream&);

#include "disk/simple_types.m"
#include "disk/pointers.m"
#include "disk/structs.m"
#include "disk/containers.m"

}
