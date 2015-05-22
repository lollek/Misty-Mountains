#ifndef _ROGUE14_IO_H_
#define _ROGUE14_IO_H_

#include <stdbool.h>
#include <curses.h>

#define MAXSTR 1024 /* maximum length of strings */
#define MAXINP   50 /* max string to read from terminal or environment */
#define MAXLINES 32 /* maximum number of screen lines used */
#define MAXCOLS  80 /* maximum number of screen columns used */
#define NUMLINES 24
#define NUMCOLS  80
#define STATLINE (NUMLINES - 1)


#undef CTRL
#define CTRL(c) (c & 037)
#define UNCTRL(c) (c + 'A' - CTRL('A'))

/* Extra named keys for curses */
#define KEY_SPACE	' '
#define KEY_ESCAPE	27

/* Chars for things */
#define SHADOW		' '
#define VWALL		'|'
#define HWALL		'-'
#define PASSAGE		'#'
#define DOOR		'+'
#define FLOOR		'.'
#define PLAYER		'@'
#define TRAP		'^'
#define STAIRS		'%'
#define GOLD		'*'
#define POTION		'!'
#define SCROLL		'?'
#define MAGIC		'$'
#define FOOD		':'
#define WEAPON		')'
#define ARMOR		']'
#define AMULET		','
#define RING		'='
#define STICK		'/'

/* Variables, TODO: Remove these */
char huh[MAXSTR];
WINDOW* hw;/* used as a scratch window */
int mpos;

void fatal(char const* msg, ...); /* Kill program with an error message */

char const* get_homedir(void); /* returns e.g. /home/user/ */

bool step_ok(int ch);  /* True of it's OK to step on ch */
void status(void);     /* Print the status line at the bottom of the screen */
void show_win(char const* message); /* Show window and wait before returning */

/* get input */
bool wreadstr(WINDOW* win, char* buf);   /* interruptable string from user */
#define readstr(_b) wreadstr(stdscr, _b) /* wreadstr for stdscr */
char readchar(bool is_question);   /* Interruptable getch() */
void wait_for(int ch); /* Wait for the specified key */

#ifdef NDEBUG
#define fail(err, ...) 1
#else
bool fail(char const* fmt, ...);
#endif /* NDEBUG */

/* Message player */
int clearmsg(void);            /* Remove displayed text */
int msg(char const* fmt, ...); /* Display a message at the top of the screen. */
void addmsg(char const* fmt, ...);       /* Add things to the current message */

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
