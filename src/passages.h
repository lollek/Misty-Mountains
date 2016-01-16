#pragma once

#include "Coordinate.h"
#include "level_rooms.h"

/* upper limit on number of passages */
#define PASSAGES_MAX 12
extern struct room passages[PASSAGES_MAX];

void passages_add_pass(void);
void passages_do(void);
void passages_putpass(Coordinate* cp);
