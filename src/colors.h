#ifndef _ROGUE14_COLORS_H_
#define _ROGUE14_COLORS_H_

/* Initialize the potion color scheme for this time */
void colors_init(void);

int color_max(void);
char const* color_get(int i);
char const* color_random(void);

#endif /* _ROGUE14_COLORS_H_ */
