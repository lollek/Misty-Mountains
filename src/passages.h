#ifndef ROGUE14_PASSAGES_H
#define ROGUE14_PASSAGES_H

#include "coord.h"
#include "rooms.h"

/* upper limit on number of passages */
#define PASSAGES_MAX 12
extern struct room passages[PASSAGES_MAX];

void passages_add_pass(void);
void passages_do(void);
void passages_putpass(coord* cp);

#endif /* ROGUE14_PASSAGES_H */
