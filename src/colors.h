#ifndef _ROGUE14_COLORS_H_
#define _ROGUE14_COLORS_H_

void *__colors_ptr(void);
size_t __colors_size(void);

/* Initialize the potion color scheme for this time */
void colors_init(void);

/* Return a random color */
char *colors_random(void);

#endif /* _ROGUE14_COLORS_H_ */
