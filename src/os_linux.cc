#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#define _XOPEN_SOURCE 500
#pragma clang diagnostic pop

#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cstdlib>
#include <string>

using namespace std;

#include "things.h"
#include "io.h"

#include "os.h"

unsigned os_rand_seed;

size_t os_rand_range(size_t max) {
  return static_cast<size_t>(os_rand()) % max;
}

int os_rand_range(int max) {
  return max == 0 ? 0 : os_rand() % max;
}

int os_rand() {
  return rand_r(&os_rand_seed);
}

int os_usleep(unsigned int usec) {
  return usleep(usec);
}

string os_whoami() {
  struct passwd const* pw = getpwuid(getuid());
  return pw ? pw->pw_name : "nobody";
}
