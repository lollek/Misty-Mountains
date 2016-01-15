#pragma once

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
size_t os_rand_range(size_t max);       /* Return a number [0,max[ */
int    os_usleep(unsigned int usec);    /* Sleep for nanoseconds */
