#ifndef _ROGUE14_THINGS_H_
#define _ROGUE14_THINGS_H_

#include "rogue.h"

#define NUMTHINGS 7
struct obj_info things[NUMTHINGS];

/* Return the name of something as it would appear in an inventory. */
char *inv_name(THING *obj, bool drop);

/* Put something down */
bool drop(void);

/* Return a new thing */
THING *new_thing(void);

/* Pick an item out of a list of nitems possible objects */
unsigned pick_one(struct obj_info *start, int nitems);

/* list what the player has discovered in this game of a certain type */
void discovered(void);

#endif /* _ROGUE14_THINGS_H_ */
