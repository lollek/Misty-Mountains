template <class T>
void save(tag_type tag, T* element, std::ostream& data) {
  save(tag, *element, data);
}

template <class T>
bool load(tag_type tag, T*& element, std::istream& data) {
  element = new T;
  return load(tag, *element, data);
}

// Special case for Item, since it's pure virtual
template<>
void save<Item>(tag_type tag, Item* element, std::ostream& data);
template<>
bool load<Item>(tag_type tag, Item*& element, std::istream& data);
