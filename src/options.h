#ifndef _ROGUE14_OPTIONS_H_
#define _ROGUE14_OPTIONS_H_

#include <stdbool.h>

/* Game Options - These are set in main.c and options.c */

extern bool terse;       /* Terse output */
extern bool fight_flush; /* Flush typeahead during battle */
extern bool jump;        /* Show running as a series of jumps */
extern bool see_floor;   /* Show the lamp-illuminated floor */
extern bool passgo;      /* Follow the turnings in passageways */
extern bool use_colors;  /* Use ncurses colors */

bool option(void);

#endif /* _ROGUE14_OPTIONS_H_ */
