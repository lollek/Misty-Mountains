#pragma once

#include <list>

#include <curses.h>
#include <string.h>

#include "level_rooms.h"
#include "coordinate.h"
#include "item.h"
#include "monster.h"

class IO {
public:
  IO();
  ~IO();

  enum End {
    End
  };

  enum Attribute {
    Standout,
    Red,
    Blue,
    None
  };

  enum Tile {
    Shadow = ' ',
    Wall   = '#',
    Floor  = '.',
    OpenDoor   = '\'',
    ClosedDoor   = '+',
    Trap   = '_',
    Stairs = '%',

    VerticalBolt = '|',
    DiagonalUpBolt = '/',
    HorizontalBolt = '-',
    DiagonalDownBolt = '\\',

    Gold   = '*',
    Potion = '!',
    Scroll = '?',
    Magic  = '$',
    Food   = ':',
    Ammo   = '(',
    Weapon = ')',
    Armor  = ']',
    Amulet = ',',
    Ring   = '=',
    Wand   = '/',

    Player = '@',

  };

  template <typename T>
  void print(int x, int y, T ch, Attribute attr=None);

  template <typename T>
  void print_color(int x, int y, T ch, Attribute attr=None);

  void print_tile(Coordinate const& coord);
  void print_tile(int x, int y);

  void hide_tile(Coordinate const& coord);
  void hide_tile(int x, int y);

  void print_level_layout();

  void print_room(room const* room);
  void hide_room(room const* room);

  void print_monster(Monster* monster, Attribute attr=None);
  void print_item(Item* item);

  chtype colorize(chtype ch);

  void repeat_last_messages();
  void clear_message();
  void show_extra_screen(std::string const& message);

  void refresh();

  std::string read_string(WINDOW* win=stdscr, std::string const* initial_string=nullptr);
  void message(std::string const& message, bool force_flush=false);


  // Temp var
  std::list<std::string> last_messages;
  std::string message_buffer;
  WINDOW* extra_screen;

private:
  void print_player_vision();

  void print_tile_seen(Coordinate const& coord);
  void print_tile_discovered(Coordinate const& coord);

  void refresh_statusline();
};

#define MAXSTR 1024 // maximum length of strings
#define MAXINP   50 // max string to read from terminal or environment
#define MAXLINES 32 // maximum number of screen lines used
#define MAXCOLS  80 // maximum number of screen columns used
#define NUMLINES 24
#define NUMCOLS  80
#define STATLINE (NUMLINES - 1)

#undef CTRL
#define CTRL(c) (c & 037)
#define UNCTRL(c) (c + 'A' - CTRL('A'))

// Extra named keys for curses
#define KEY_SPACE	' '
#define KEY_ESCAPE	27

/* Encrypted read/write to/from file */
size_t io_encwrite(char const* start, size_t size, FILE* outf);
size_t io_encread(char* start, size_t size, FILE* inf);

void io_missile_motion(Item* item, int ydelta, int xdelta);


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




/* old ncurses functions, with custom color support, to be removed */
#define waddcch(_w, _c)           waddch(_w, Game::io->colorize(_c))
#define mvwaddcch(_w, _y, _x, _c) mvwaddch(_w, _y, _x, Game::io->colorize(_c))
