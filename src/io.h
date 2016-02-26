#pragma once

#include <list>

#include <string.h>

#include "level_rooms.h"
#include "coordinate.h"
#include "item.h"
#include "monster.h"
#include "tiles.h"

class IO {
public:
  IO();
  ~IO();

  enum Attribute {
    Standout,
    Red,     BoldRed,
    Green,   BoldGreen,
    Yellow,  BoldYellow,
    Blue,    BoldBlue,
    Magenta, BoldMagenta,
    Cyan,    BoldCyan,
    White,   BoldWhite,
    Black,   BoldBlack,
    None
  };

  enum Tile {
    Shadow = ' ',
    Wall   = '#',
    Floor  = '.',
    OpenDoor   = '\'',
    ClosedDoor   = '+',
    Trap   = '_',
    StairsDown = '>',
    StairsUp = '<',
    Shop = '~',

    VerticalBolt = '|',
    DiagonalUpBolt = '/',
    HorizontalBolt = '-',
    DiagonalDownBolt = '\\',

    Gold   = '$',
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

  char readchar(bool is_question);
  void wait_for_key(int ch);

  void print_level_layout();

  void print_room(room const* room);

  void missile_motion(Item* item, int ydelta, int xdelta);

  void repeat_last_messages();
  void clear_message();

  void print(int x, int y, long unsigned int ch, Attribute attr=None);
  void print_coordinate(Coordinate const& coord);
  void print_coordinate(int x, int y);
  void refresh();
  void force_redraw();

  std::string read_string(std::string const* initial_string=nullptr, bool question=true);
  void message(std::string const& message, bool force_flush=false);

  void move_pointer(int x, int y);
  void clear_screen();
  void print_char(int x, int y, char sym);
  void print_string(int x, int y, std::string const& str);
  void print_string(std::string const& str);

  void stop_curses();
  void resume_curses();

  static int constexpr map_start_x{20};
  static int constexpr map_start_y{1};
  static int constexpr map_width{80};
  static int constexpr map_height{32};

  static int constexpr screen_width{100};
  static int constexpr screen_height{35};

  static int constexpr message_x = map_start_x;
  static int constexpr message_y = 0;

  static int constexpr max_input = 50;

private:
  void print_coordinate_seen(Coordinate const& coord);
  bool print_monster(int x, int y, Monster* monster);
  bool print_item(int x, int y, Item* item);
  void print_tile(int x, int y, ::Tile::Type tile);

  void refresh_statusline();

  std::list<std::string> last_messages;
  std::string message_buffer;
};

#undef CTRL
#define CTRL(c) (c & 037)
#define UNCTRL(c) (c + 'A' - CTRL('A'))

// Extra named keys for curses
#define KEY_SPACE	' '
#define KEY_ESCAPE	27

// Encrypted read/write to/from file
size_t io_encwrite(char const* start, size_t size, FILE* outf);
size_t io_encread(char* start, size_t size, FILE* inf);


