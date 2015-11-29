#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#define _XOPEN_SOURCE 500
#pragma clang diagnostic pop

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

item*
os_calloc_item(void)
{
  item* item = calloc(1, sizeof *item);

  if (item == NULL)
    io_fatal("No virtual memory available!\n");

  assert(item->l_next == NULL);
  assert(item->l_prev == NULL);

  return item;
}

monster*
os_calloc_monster(void)
{
  monster* monster = calloc(1, sizeof *monster);

  if (monster == NULL)
    io_fatal("No virtual memory available!\n");

  assert(monster->l_next == NULL);
  assert(monster->l_prev == NULL);

  return monster;
}


void
os_remove_thing(THING** thing)
{
  assert(thing != NULL);

  free(*thing);
  *thing = NULL;
}

void
os_remove_item(item** item)
{
  assert(item != NULL);

  free(*item);
  *item = NULL;
}

void
os_remove_monster(monster** monster)
{
  assert(monster != NULL);

  free(*monster);
  *monster = NULL;
}
