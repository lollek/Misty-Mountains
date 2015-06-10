#ifndef _ROGUE14_OS_H_
#define _ROGUE14_OS_H_

#include <sys/types.h>

#include "things.h"

extern unsigned os_rand_seed;

int    os_rand(void);                   /* Return a pseudorandom number */
int    os_rand_range(int max);          /* Return a number [0,max[ */
int    os_usleep(unsigned int usec);    /* Sleep for nanoseconds */
int    os_drop_uid(void);               /* Revert to true user id */
int    os_drop_gid(void);               /* Revert to true group id */
THING* os_calloc_thing(void);           /* Malloc and memset a THING */
void   os_remove_thing(THING** thing);  /* Free a THING and set it to NULL */


#endif /* _ROGUE14_OS_H_ */
