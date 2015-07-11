#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include "things.h"
#include "io.h"

#include "os.h"

unsigned os_rand_seed;

int
os_rand_range(int max)
{
  return max == 0 ? 0 : os_rand() % max;
}

int
os_rand(void)
{
  return rand_r(&os_rand_seed);
}

int
os_usleep(unsigned int usec)
{
  return usleep(usec);
}

int
os_drop_uid(void)
{
  uid_t realuid = getuid();
  return setreuid(realuid, realuid);
}

int
os_drop_gid(void)
{
  gid_t realgid = getgid();
  return setregid(realgid, realgid);
}

THING*
os_calloc_thing(void)
{
  THING* item = calloc(1, sizeof *item);

  if (item == NULL)
    io_fatal("No virtual memory available!\n");

  assert(item->o.l_next == NULL);
  assert(item->o.l_prev == NULL);

  return item;
}

void os_remove_thing(THING** thing)
{
  assert(thing != NULL);

  free(*thing);
  *thing = NULL;
}
