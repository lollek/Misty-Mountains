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
  S_ID_POTION = 5,
  S_ID_SCROLL = 6,
  S_ID_WEAPON = 7,
  S_ID_ARMOR  = 8,
  S_ID_R_OR_S = 9,
  S_SCARE     = 10,
  S_FDET      = 11,
  S_TELEP     = 12,
  S_ENCH      = 13,
  S_CREATE    = 14,
  S_REMOVE    = 15,
  S_AGGR      = 16,
  S_PROTECT   = 17,
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
