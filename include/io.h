#pragma once

#include <list>

#include <string.h>

#include "level_rooms.h"
#include "coordinate.h"
#include "item.h"
#include "monster.h"
#include "tiles.h"

#undef CTRL
#define CTRL(c) (c & 037)
#define UNCTRL(c) (c + 'A' - CTRL('A'))

// Extra named keys for curses
#define KEY_SPACE	' '
#define KEY_ESCAPE	27

class IO {
public:
  IO();
  ~IO();

  enum Attribute {
    Standout,
    Red,     BoldRed,     Green,   BoldGreen,
    Yellow,  BoldYellow,  Blue,    BoldBlue,
    Magenta, BoldMagenta, Cyan,    BoldCyan,
    White,   BoldWhite,   Black,   BoldBlack,
    None
  };

  enum Tile {
    // Tiles
    Shadow     = ' ',  Wall       = '#', Floor = '.',
    OpenDoor   = '\'', ClosedDoor = '+', Trap  = '_',
    StairsDown = '>',  StairsUp   = '<', Shop  = '~',

    // Bolts
    VerticalBolt   = '|', DiagonalUpBolt   = '/',
    HorizontalBolt = '-', DiagonalDownBolt = '\\',

    // Items
    Gold   = '$', Potion = '!', Scroll = '?', Magic  = '$',
    Food   = ':', Ammo   = '(', Weapon = ')', Armor  = ']',
    Amulet = ',', Ring   = '=', Wand   = '/',

    // Monsters :)
    Player = '@',
  };

  // Messages
  void message(std::string const& message, bool force_flush=false);
  void clear_message();
  void repeat_last_messages();

  // Read data
  std::string read_string(std::string const* initial_string=nullptr, bool question=true);
  char readchar(bool is_question);
  void wait_for_key(int ch);

  // Misc IO
  void missile_motion(Item* item, int ydelta, int xdelta);
  void character_creation();

  // Refresh what we see
  void refresh();
  void print(int x, int y, long unsigned int ch, Attribute attr=None);

  // Raw functions
  void move_pointer(int x, int y);
  void clear_screen();
  void print_char(int x, int y, char sym);
  void print_string(int x, int y, std::string const& str);
  void print_string(std::string const& str);
  void force_redraw();

  // Start/stop
  void stop_curses();
  void resume_curses();

  // Map
  static int constexpr map_start_x{20};
  static int constexpr map_start_y{1};
  static int constexpr map_width{80};
  static int constexpr map_height{32};

  // Screen
  static int constexpr screen_width{100};
  static int constexpr screen_height{35};

  // Message
  static int constexpr message_x = map_start_x;
  static int constexpr message_y = 0;

  // Input
  static int constexpr max_input = 50;

private:
  // Refresh
  void print_coordinate(Coordinate const& coord);
  void print_coordinate(int x, int y);
  void print_coordinate_seen(Coordinate const& coord);
  bool print_monster(int x, int y, Monster* monster);
  bool print_item(int x, int y, Item* item);
  void print_tile(int x, int y, ::Tile::Type tile);

  void refresh_statusline();

  std::list<std::string> last_messages;
  std::string message_buffer;
};

// Inlines
inline void IO::print_coordinate(Coordinate const& coord) {
  print_coordinate(coord.x, coord.y);
}
