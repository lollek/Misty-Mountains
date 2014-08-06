#ifndef _ROGUE14_SCROLLS_H_
#define _ROGUE14_SCROLLS_H_

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
extern char *s_names[NSCROLLS];            /* Names of the scrolls */
extern struct obj_info scr_info[NSCROLLS]; /* Scroll info */

/* Functions */
void read_scroll(void);     /* Read a scroll from the pack and do the needful */
void uncurse(THING *obj);   /* Uncurse an item */
#define learn_scroll(_s) (scr_info[_s].oi_know = true) /* Learn scroll info */
#define knows_scroll(_s) (scr_info[_s].oi_know) /* Knows what scroll does? */

#endif /* _ROGUE14_SCROLLS_H_ */
