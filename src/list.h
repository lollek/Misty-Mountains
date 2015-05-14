#ifndef _ROGUE14_LIST_H_
#define _ROGUE14_LIST_H_

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

/* Throw the whole blamed thing away */
void _free_list(THING **ptr);

/* Free up an item */
void _discard(THING **item);

/* Get a new item with a specified size */
THING * new_item(void);

#endif /* _ROGUE14_LIST_H_ */
