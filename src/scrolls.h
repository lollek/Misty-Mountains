#ifndef _ROGUE14_SCROLLS_H_
#define _ROGUE14_SCROLLS_H_

#include <stdbool.h>

#include "things.h"

/* Scroll types */
enum scroll_t
{
  S_CONFUSE   = 0,
  S_MAP       = 1,
  S_HOLD      = 2,
  S_SLEEP     = 3,
  S_ARMOR     = 4,
  S_ID        = 5,
  S_SCARE     = 6,
  S_FDET      = 7,
  S_TELEP     = 8,
  S_ENCH      = 9,
  S_CREATE    = 10,
  S_REMOVE    = 11,
  S_AGGR      = 12,
  S_PROTECT   = 13,
  NSCROLLS
};

/* Variables */
extern struct obj_info scroll_info[NSCROLLS]; /* Scroll info */

void scroll_init(void);
bool scroll_save_state(void);
bool scroll_load_state(void);

/* Functions */
bool scroll_read(void);     /* Read a scroll from the pack and do the needful */
void scroll_learn(enum scroll_t scroll);    /* Learn scroll info */
bool scroll_is_known(enum scroll_t scroll); /* Knows what scroll does? */
int scroll_value(enum scroll_t scroll);
void scroll_set_name(enum scroll_t wand, char const* new_name);

void scroll_description(item const* item, char* buf);

THING* scroll_create(int which);

#endif /* _ROGUE14_SCROLLS_H_ */
