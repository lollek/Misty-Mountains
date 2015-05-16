#ifndef _ROGUE14_LIST_H_
#define _ROGUE14_LIST_H_

#include <stdint.h>

#include "things.h"

#define assert_monster(a) assert_attached(mlist, a)

#ifndef NDEBUG
#define assert_attached(a, b) _assert_attached(a, b)
void _assert_attached(THING *list, THING *item);
#else
#define assert_attached(a, b)
#endif

/* takes an item out of whatever linked list it might be in */
void list_detach(THING **list, THING *item);

/* add an item to the head of a list */
void list_attach(THING **list, THING *item);

/* Find index of thing in list */
int8_t list_find(THING const* list, THING const* thing);

/* Returns item with index i */
THING *list_element(THING *list, int8_t i);

/* Throw the whole blamed thing away */
void list_free_all(THING **ptr);

/* Free up an item */
void _discard(THING **item);

#endif /* _ROGUE14_LIST_H_ */
