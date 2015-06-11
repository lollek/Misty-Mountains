#ifndef _ROGUE14_ITEM_H_
#define _ROGUE14_ITEM_H_

#include "things.h"

/* Linked list pointers */
static inline union thing* item_next(item const* item)
{ return item->l_next; }
static inline union thing* item_prev(item const* item)
{ return item->l_prev; }

/* Data */
static inline coord* item_pos(item* item)
{ return &item->o_pos; }
static inline char* item_nickname(item* item)
{ return item->o_label; }
static inline int item_type(item const* item)
{ return item->o_type; }
static inline int item_subtype(item const* item)
{ return item->o_which; }
static inline int item_count(item const* item)
{ return item->o_count; }


#endif /* _ROGUE14_ITEM_H_ */
