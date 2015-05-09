#ifndef _ROGUE14_LEVEL_H_
#define _ROGUE14_LEVEL_H_

/* TODO: Make functions of these */
#define INDEX(y,x)	(&places[((x) << 5) + (y)])
#define chat(y,x)	(places[((x) << 5) + (y)].p_ch)
#define flat(y,x)	(places[((x) << 5) + (y)].p_flags)
#define moat(y,x)	(places[((x) << 5) + (y)].p_monst)
PLACE places[MAXLINES*MAXCOLS];  /* level map */

THING *lvl_obj; /* List of objects on this level */

void level_new(void);

#endif /* _ROGUE14_LEVEL_H_ */
