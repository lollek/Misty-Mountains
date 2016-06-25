#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <string>

using namespace std;

#include "io.h"

#include "os.h"

unsigned os_rand_seed;

size_t os_rand_range(size_t max) {
  return static_cast<size_t>(os_rand()) % max;
}

int os_rand_range(int max) { return max == 0 ? 0 : os_rand() % max; }

int os_rand() { return rand_r(&os_rand_seed); }

int os_process_id() { return getpid(); }

int os_usleep(unsigned int usec) { return usleep(usec); }

string os_whoami() {
  struct passwd const* pw{getpwuid(getuid())};
  return pw ? pw->pw_name : "nobody";
}

string os_homedir() {
  string return_value;

  // Try to get it from pwuid
  struct passwd const* pw{getpwuid(getuid())};
  char const* ptr{getenv("HOME")};

  if (pw != nullptr) {
    return_value = pw->pw_dir;

    // Try $HOME
  } else if (ptr != nullptr) {
    return_value = ptr;
  }

  // Otherwise, we just return current directory
  if (return_value.empty()) {
    return_value = "/";
  }

  if (return_value.back() != '/') {
    return_value += '/';
  }

  return return_value;
}
