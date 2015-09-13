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

int    os_rand(void);                   /* Return a pseudorandom number */
int    os_rand_range(int max);          /* Return a number [0,max[ */
int    os_usleep(unsigned int usec);    /* Sleep for nanoseconds */
THING* os_calloc_thing(void);           /* Malloc and memset a THING */
void   os_remove_thing(THING** thing);  /* Free a THING and set it to NULL */


#endif /* ROGUE14_OS_H */
