template <template <class, class> class C, class T>
void save(tag_type tag, C<T, std::allocator<T>> const& container, std::ofstream& data) {
  save_tag(tag, data);

  size_t size = container.size();
  save(tag, size, data);

  for (T const& element : container) {
    save(tag, element, data);
  }
}
template <template <class, class> class C, class T>
bool load(tag_type tag, C<T, std::allocator<T>>& container, std::ifstream& data) {
  if (!load_tag(tag, data)) { return false; }

  size_t size;
  if (!load(tag, size, data)) { return false; }
  container.resize(size);

  for (T& element : container) {
    if (!load(tag, element, data)) { return false; }
  }
  return true;
}


// Special case for vector<bool> since it wanna feel special
template <>
void save<std::vector, bool>(tag_type, std::vector<bool> const&, std::ofstream&);
template <>
bool load<std::vector, bool>(tag_type, std::vector<bool>&, std::ifstream&);
