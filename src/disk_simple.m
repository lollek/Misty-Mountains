template <class T>
void save(tag_type tag, T const& element, std::ostream& data) {
  static_assert(std::is_fundamental<T>::value, "Not fundamental type T");
  save_tag(tag, data);
  data.write(reinterpret_cast<char const*>(&element), sizeof(element));
}

template <class T>
bool load(tag_type tag, T& element, std::istream& data) {
  static_assert(std::is_fundamental<T>::value, "Not fundamental type T");
  if (!load_tag(tag, data)) { return false; }
  data.read(reinterpret_cast<char*>(&element), sizeof(element));
  return true;
}

// std::string
template <>
void save<std::string>(tag_type tag, std::string const& element, std::ostream& data);
template <>
bool load<std::string>(tag_type tag, std::string& element, std::istream& data);

// Item
template <>
void save<Item>(tag_type tag, Item const& element, std::ostream& data);
template <>
bool load<Item>(tag_type tag, Item& element, std::istream& data);

// Feat
template <>
void save<Character::Feat>(tag_type tag, Character::Feat const& element, std::ostream& data);
template <>
bool load<Character::Feat>(tag_type tag, Character::Feat& element, std::istream& data);
