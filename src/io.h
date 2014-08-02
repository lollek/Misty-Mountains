#ifndef _ROGUE14_IO_H_
#define _ROGUE14_IO_H_

bool step_ok(int ch);  /* True of it's OK to step on ch */
void status(void);     /* Print the status line at the bottom of the screen */
void show_win(const char *message); /* Show window and wait before returning */

/* get input */
bool wreadstr(WINDOW *win, char *buf);  /* interruptable string from user */
inline bool readstr(char *buf);         /* wreadstr for stdscr */
char readchar(void);   /* Interruptable getch() */
void wait_for(int ch); /* Wait for the specified key */

/* Message player */
int msg(const char *fmt, ...); /* Display a message at the top of the screen. */
void addmsg(const char *fmt, ...);       /* Add things to the current message */
int endmsg(void); /* Flush message */

/* Helper function to colorize chars before outputting them */
chtype colorize(const chtype ch);

/* ncurses functions, but with custom color support */
inline chtype incch(void);
inline chtype wincch(WINDOW *win);
inline chtype mvincch(int y, int x);
inline chtype mvwincch(WINDOW *win, int y, int x);
inline int addcch(const chtype ch);
inline int waddcch(WINDOW *window, const chtype ch);
inline int mvaddcch(int y, int x, const chtype ch);
inline int mvwaddcch(WINDOW *window, int y, int x, const chtype ch);

#endif /* _ROGUE14_IO_H_ */
