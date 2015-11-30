#ifndef ROGUE14_OS_H
#define ROGUE14_OS_H

#include <sys/types.h>

#ifdef __APPLE__
#  include <sys/syslimits.h>
#elif __linux
#  include <linux/limits.h>
#endif

#include "things.h"

extern unsigned os_rand_seed;

int os_rand(void);                   /* Return a pseudorandom number */
int os_rand_range(int max);          /* Return a number [0,max[ */
int os_usleep(unsigned int usec);    /* Sleep for nanoseconds */

/* Malloc and memset a struct */
THING*   os_calloc_thing(void);
item*    os_calloc_item(void);
monster* os_calloc_monster(void);

/* Free and set it to NULL */
void os_remove_thing(THING** thing);
void os_remove_item(item** item);
void os_remove_monster(monster** monster);

/* Temporary functions for transforming */
THING* os_item_to_thing(item** item);
THING* os_monster_to_thing(monster** monster);

#endif /* ROGUE14_OS_H */
