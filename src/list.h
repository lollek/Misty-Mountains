#ifndef ROGUE14_LIST_H
#define ROGUE14_LIST_H

#include <stdint.h>

#include "things.h"

#ifndef NDEBUG
#define list_assert_attached(a, b) _list_assert_attached(a, b)
void _list_assert_attached(THING const* list, THING const* item);
#else
#define list_assert_attached(a, b)
#endif

/* takes an item out of whatever linked list it might be in */
void list_detach(THING** list, THING* item);

/* add an item to the head of a list */
void list_attach(THING** list, THING* item);

/* Find index of thing in list */
int8_t list_find(THING const* list, THING const* thing);

/* Returns item with index i */
THING const* list_element(THING const* list, int8_t i);

/* Throw the whole blamed thing away */
void list_free_all(THING** ptr);

#endif /* ROGUE14_LIST_H */
