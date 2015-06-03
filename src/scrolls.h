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
extern char* s_names[NSCROLLS];            /* Names of the scrolls */
extern struct obj_info scr_info[NSCROLLS]; /* Scroll info */

void scroll_init(void);
bool scroll_save_state(void);
bool scroll_load_state(void);

/* Functions */
void identify(void);        /* Identify something from player's pack */
bool read_scroll(void);     /* Read a scroll from the pack and do the needful */
#define learn_scroll(_s) (scr_info[_s].oi_know = true) /* Learn scroll info */
#define knows_scroll(_s) (scr_info[_s].oi_know) /* Knows what scroll does? */

void scroll_description(THING* obj, char* buf);

THING* scroll_create(int which);

#endif /* _ROGUE14_SCROLLS_H_ */
