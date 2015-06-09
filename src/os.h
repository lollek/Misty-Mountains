#ifndef _ROGUE14_OS_H_
#define _ROGUE14_OS_H_

#include <sys/types.h>

#include "things.h"

int rand_r(unsigned* seedp);
int usleep(unsigned int usec);

int    os_drop_uid(void);
int    os_drop_gid(void);
THING* os_calloc_thing(void);
void   os_remove_thing(THING** thing);


#endif /* _ROGUE14_OS_H_ */
