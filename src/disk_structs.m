//Daemons::Daemon
template<>
void save<Daemons::Daemon>(tag_type tag, Daemons::Daemon const& element,
                           std::ofstream& data);
template<>
bool load<Daemons::Daemon>(tag_type tag, Daemons::Daemon& element, std::ifstream& data);

//Daemons::Fuse
template<>
void save<Daemons::Fuse>(tag_type tag, Daemons::Fuse const& element, std::ofstream& data);
template<>
bool load<Daemons::Fuse>(tag_type tag, Daemons::Fuse& element, std::ifstream& data);
