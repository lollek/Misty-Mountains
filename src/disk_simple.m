template <class T>
void save(tag_type tag, T const& element, std::ofstream& data) {
  static_assert(std::is_fundamental<T>::value, "Not fundamental type T");
  save_tag(tag, data);
  data.write(reinterpret_cast<char const*>(&element), sizeof(element));
}

template <class T>
bool load(tag_type tag, T& element, std::ifstream& data) {
  static_assert(std::is_fundamental<T>::value, "Not fundamental type T");
  if (!load_tag(tag, data)) { return false; }
  data.read(reinterpret_cast<char*>(&element), sizeof(element));
  return true;
}

// std::string
template <>
void save<std::string>(tag_type tag, std::string const& element, std::ofstream& data);
template <>
bool load<std::string>(tag_type tag, std::string& element, std::ifstream& data);

// item
template <>
void save<Item>(tag_type tag, Item const& element, std::ofstream& data);
template <>
bool load<Item>(tag_type tag, Item& element, std::ifstream& data);
