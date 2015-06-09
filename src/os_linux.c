#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include "things.h"
#include "io.h"

#include "os.h"

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
    fatal("No virtual memory available!\n");

  assert(item->l_next == NULL);
  assert(item->l_prev == NULL);

  return item;
}

void os_remove_thing(THING** thing)
{
  assert(thing != NULL);

  free(*thing);
  *thing = NULL;
}
