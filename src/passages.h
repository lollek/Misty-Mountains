#ifndef _ROGUE14_PASSAGES_H_
#define _ROGUE14_PASSAGES_H_

#include "coord.h"
#include "rooms.h"

#define MAXPASS		13	/* upper limit on number of passages */
struct room passages[MAXPASS];

void passages_add_pass(void);
void passages_do(void);
void passages_putpass(coord *cp);

#endif /* _ROGUE14_PASSAGES_H_ */
