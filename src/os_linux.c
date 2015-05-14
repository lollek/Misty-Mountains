#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include "rogue.h"

THING *
allocate_new_item(void)
{
  THING *item = calloc(1, sizeof *item);

  if (item == NULL)
    fatal("No virtual memory available!\n");

  assert(item->l_next == NULL);
  assert(item->l_prev == NULL);

  return item;
}

#include "os.h"
