#ifndef _ROGUE14_OS_H_
#define _ROGUE14_OS_H_

#include <sys/types.h>

typedef unsigned int usecs;
int rand_r(unsigned *seedp);
int usleep(usecs usec);
int setreuid(uid_t ruid, uid_t euid);
int setregid(gid_t rgid, gid_t egid);

#endif /* _ROGUE14_OS_H_ */
