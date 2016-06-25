// Daemons::Daemon
template <>
void save<Daemons::Daemon>(tag_type tag, Daemons::Daemon const& element,
                           std::ostream& data);
template <>
bool load<Daemons::Daemon>(tag_type tag, Daemons::Daemon& element,
                           std::istream& data);

// Daemons::Fuse
template <>
void save<Daemons::Fuse>(tag_type tag, Daemons::Fuse const& element,
                         std::ostream& data);
template <>
bool load<Daemons::Fuse>(tag_type tag, Daemons::Fuse& element,
                         std::istream& data);

// Coordinate
template <>
void save<Coordinate>(tag_type tag, Coordinate const& element,
                      std::ostream& data);
template <>
bool load<Coordinate>(tag_type tag, Coordinate& element, std::istream& data);

// damage
template <>
void save<damage>(tag_type tag, damage const& element, std::ostream& data);
template <>
bool load<damage>(tag_type tag, damage& element, std::istream& data);
