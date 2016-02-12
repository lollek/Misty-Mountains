template <class T>
void save(tag_type tag, T* element, std::ofstream& data) {
  save(tag, *element, data);
}

template <class T>
bool load(tag_type tag, T*& element, std::ifstream& data) {
  element = new T;
  return load(tag, *element, data);
}

// Special case for Item, since it's pure virtual
template<>
void save<Item>(tag_type tag, Item* element, std::ofstream& data);
template<>
bool load<Item>(tag_type tag, Item*& element, std::ifstream& data);
