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
os_item_to_thing(item** item)
{
  assert(item != NULL);
  assert(*item != NULL);

  THING* return_value = new THING();
  return_value->o = **item;
  delete *item;
  *item = nullptr;

  return return_value;
}

THING* os_monster_to_thing(monster** monster)
{
  assert(monster != NULL);
  assert(*monster != NULL);

  THING* return_value = new THING();
  return_value->t = **monster;
  delete *monster;
  *monster = nullptr;

  return return_value;
}
