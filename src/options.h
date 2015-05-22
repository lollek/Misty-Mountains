#ifndef _ROGUE14_OPTIONS_H_
#define _ROGUE14_OPTIONS_H_

#include <stdbool.h>

/* Game Options - These are set in main.c and options.c */

bool terse;       /* Terse output */
bool fight_flush; /* Flush typeahead during battle */
bool jump;        /* Show running as a series of jumps */
bool see_floor;   /* Show the lamp-illuminated floor */
bool passgo;      /* Follow the turnings in passageways */
bool use_colors;  /* Use ncurses colors */

bool option(void);

#endif /* _ROGUE14_OPTIONS_H_ */
