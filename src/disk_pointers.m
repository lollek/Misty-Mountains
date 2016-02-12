template <class T>
void save(tag_type tag, T* element, std::ofstream& data) {
  save(tag, *element, data);
}

template <class T>
bool load(tag_type tag, T** element, std::ifstream& data) {
  *element = new T;
  return load(tag, **element, data);
}
