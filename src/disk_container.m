
template <template <class, class> class C, class T>
void save(tag_type tag, C<T, std::allocator<T>>* container, std::ofstream& data) {
  save_tag(tag, data);

  size_t size = container->size();
  save(tag, size, data);

  for (T const& element : *container) {
    save(tag, element, data);
  }
}
template <template <class, class> class C, class T>
bool load(tag_type tag, C<T, std::allocator<T>>** container, std::ifstream& data) {
  if (!load_tag(tag, data)) {
    return false;
  }

  size_t size;
  load(tag, size, data);
  *container = new C<T, std::allocator<T>>();

  for (size_t i = 0; i < size; ++i) {
    T new_value;
    load(tag, new_value, data);
    (*container)->push_back(new_value);
  }
  return true;
}

