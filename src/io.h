#ifndef _ROGUE14_IO_H_
#define _ROGUE14_IO_H_

#include <stdbool.h>
#include <curses.h>

const char *get_homedir(void); /* returns e.g. /home/user/ */

bool step_ok(int ch);  /* True of it's OK to step on ch */
void status(void);     /* Print the status line at the bottom of the screen */
void show_win(const char *message); /* Show window and wait before returning */

/* get input */
bool wreadstr(WINDOW *win, char *buf);   /* interruptable string from user */
#define readstr(_b) wreadstr(stdscr, _b) /* wreadstr for stdscr */
char readchar(void);   /* Interruptable getch() */
void wait_for(int ch); /* Wait for the specified key */

/* Message player */
int msg(const char *fmt, ...); /* Display a message at the top of the screen. */
void addmsg(const char *fmt, ...);       /* Add things to the current message */
int endmsg(void); /* Flush message */

/* Helper function to colorize chars before outputting them */
chtype colorize(const chtype ch);

/* ncurses functions, but with custom color support */
#define incch()              (inch() & A_CHARTEXT)
#define wincch(_w)           (winch(_w) & A_CHARTEXT)
#define mvincch(_y, _x)      (mvinch(_y, _x) & A_CHARTEXT)
#define mvwincch(_w, _y, _x) (mvwinch(_w, _y, _x) & A_CHARTEXT)

#define addcch(_c)                addch(colorize(_c))
#define waddcch(_w, _c)           waddch(_w, colorize(_c))
#define mvaddcch(_y, _x, _c)      mvaddch(_y, _x, colorize(_c))
#define mvwaddcch(_w, _y, _x, _c) mvwaddch(_w, _y, _x, colorize(_c))

#endif /* _ROGUE14_IO_H_ */
