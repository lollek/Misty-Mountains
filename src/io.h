#ifndef ROGUE14_IO_H
#define ROGUE14_IO_H

#include <stdbool.h>
#include <curses.h>

#include "things.h"

#define GAME_VERSION "Rogue14 " VERSION " - Based on Rogue5.4.4"

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

typedef enum attribute {
  ATTR_FIRE,
  ATTR_ICE,
  ATTR_NONE,
} attribute;

typedef enum tile {
  TILE_BOLT_VERTICAL,
  TILE_BOLT_DIAGUP,
  TILE_BOLT_HORIZONTAL,
  TILE_BOLT_DIAGDOWN,
  TILE_ERROR,
} tile;

/* Glyphs for things */
typedef chtype glyph;
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
#define AMMO		'('
#define WEAPON		')'
#define ARMOR		']'
#define AMULET		','
#define RING		'='
#define STICK		'/'

/* TODO: (Re)move these */
extern WINDOW* hw;       /* used as a scratch window */
char const* get_homedir(void); /* returns e.g. /home/user/ */
bool step_ok(int ch);  /* True of it's OK to step on ch */
/* Show window and wait before returning */
void show_win(char const* message); 

/* Encrypted read/write to/from file */
size_t io_encwrite(char const* start, size_t size, FILE* outf);
size_t io_encread(char* start, size_t size, FILE* inf);

/* Messages on the top line of the screen */
void io_msg(char const* fmt, ...);         /* Display a message */
void io_msg_unsaved(char const* fmt, ...); /* Unsaved msg() */
void io_msg_add(char const* fmt, ...);     /* Add text to previous message */
void io_msg_last(void);                    /* Reshow last msg */
void io_msg_clear(void);                   /* Remove displayed text */

void io_missile_motion(THING* obj, int ydelta, int xdelta);

/* Kill program with an error message */
void io_fatal(char const* msg, ...) __attribute__((noreturn));

/* Print the status line at the bottom of the screen */
void io_refresh_statusline(void);

/* Interruptable read string from user */
bool io_wreadstr(WINDOW* win, char* buf);
static inline bool io_readstr(char* dest)
{ return io_wreadstr(stdscr, dest); }

/* Interruptable read char from user (getch) */
char io_readchar(bool is_question);

/* Wait for the specified key */
void io_wait_for_key(int ch);

#ifdef NDEBUG
#  define io_fail(err, ...) 1
#  define io_debug(err, ...)
#  define io_debug_fatal(err, ...)
#else
  /* Print debug message (if debug mode) and return 1 */
  bool io_fail(char const* fmt, ...);
  /* Print debug message (if debug mode) or do nothing */
  void io_debug(char const* fmt, ...);
  /* Print debug message and crash (if debug mode) or do nothing */
  void io_debug_fatal(char const* fmt, ...);
#endif /* NDEBUG */




/* Helper function to colorize chars before outputting them */
chtype colorize(const chtype ch);

/* old ncurses functions, with custom color support, to be removed */
#define incch()              (inch() & A_CHARTEXT)
#define wincch(_w)           (winch(_w) & A_CHARTEXT)
#define mvincch(_y, _x)      (mvinch(_y, _x) & A_CHARTEXT)
#define mvwincch(_w, _y, _x) (mvwinch(_w, _y, _x) & A_CHARTEXT)

#define addcch(_c)                addch(colorize(_c))
#define waddcch(_w, _c)           waddch(_w, colorize(_c))
#define mvaddcch(_y, _x, _c)      mvaddch(_y, _x, colorize(_c))
#define mvwaddcch(_w, _y, _x, _c) mvwaddch(_w, _y, _x, colorize(_c))



/* New kind of IO API for better abstraction */
chtype io_attribute(enum attribute attribute);
chtype io_tile(enum tile tile);

static inline int
io_addch(enum tile tile, enum attribute attr)
{ return addch(io_tile(tile) | io_attribute(attr)); }

static inline int
io_waddch(WINDOW* win, enum tile tile, enum attribute attr)
{ return waddch(win, io_tile(tile) | io_attribute(attr)); }

static inline int
io_mvaddch(int y, int x, enum tile tile, enum attribute attr)
{ return mvaddch(y, x, io_tile(tile) | io_attribute(attr)); }

static inline int
io_mvwaddch(WINDOW* win, int y, int x, enum tile tile, enum attribute attr)
{ return mvwaddch(win, y, x, io_tile(tile) | io_attribute(attr)); }

#endif /* ROGUE14_IO_H */
