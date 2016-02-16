#include <sstream>
#include <string>

#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <curses.h>

#include "error_handling.h"
#include "game.h"
#include "armor.h"
#include "command.h"
#include "food.h"
#include "level.h"
#include "misc.h"
#include "options.h"
#include "os.h"
#include "player.h"
#include "rogue.h"

#include "io.h"

using namespace std;

IO::IO() : last_messages(), message_buffer(), extra_screen(nullptr) {
  initscr();  // Start up cursor package

  // Ncurses colors
  if (use_colors) {
    if (start_color() == ERR) {
      endwin();
      cerr
        << "Error: Failed to start colors. "
        << "Try restarting without colors enabled\n";
      Game::exit();
    }

    // Because ncurses has defined COLOR_BLACK to 0 and COLOR_WHITE to 7,
    // and then decided that init_pair cannot change number 0 (COLOR_BLACK)
    // I use COLOR_WHITE for black text and COLOR_BLACK for white text

    assume_default_colors(0, -1); // Default is white text and any background
    init_pair(COLOR_RED, COLOR_RED, -1);
    init_pair(COLOR_GREEN, COLOR_GREEN, -1);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, -1);
    init_pair(COLOR_BLUE, COLOR_BLUE, -1);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, -1);
    init_pair(COLOR_CYAN, COLOR_CYAN, -1);
    init_pair(COLOR_WHITE, COLOR_BLACK, -1);
  }

  if (LINES < NUMLINES || COLS < NUMCOLS) {
    endwin();
    cerr << "\nSorry, the screen must be at least "
         << NUMLINES << "x" << NUMCOLS << "\n";
    Game::exit();
  }

  raw();     // Raw mode
  noecho();  // Echo off

  extra_screen = newwin(LINES, COLS, 0, 0);
}

IO::~IO() {
  delwin(extra_screen);
  endwin();
}

char IO::readchar(bool is_question) {
  if (is_question) {
    move(0, static_cast<int>(Game::io->message_buffer.size()));
  }

  char ch = static_cast<char>(getch());
  switch (ch) {
    case 3:
      command_signal_quit(0);
      return KEY_ESCAPE;

    default:
      return ch;
  }
}

void IO::wait_for_key(int ch) {
  switch (ch) {
    case KEY_ENTER: case '\n':
      for (;;)
        if ((ch = readchar(true)) == '\n' || ch == '\r')
          return;

    default:
      for (;;)
        if (readchar(true) == ch)
          return;
  }
}





void IO::print_monster(Monster* monster, IO::Attribute attr) {
  char symbol_to_print = monster->get_disguise();
  Coordinate const& coord = monster->get_position();
  print_color(coord.x, coord.y, symbol_to_print, attr);
}

void IO::print_item(Item* item) {
  int symbol_to_print = item->get_item_type();
  Coordinate const& coord = item->get_position();
  print_color(coord.x, coord.y, symbol_to_print);
}

void IO::print_tile_seen(Coordinate const& coord) {
  Game::level->set_discovered(coord);

  // Next prio: Player
  if (player->get_position() == coord) {
    print_color(coord.x, coord.y, player->get_type());
    return;
  }

  // Next prio: Monsters
  Monster* mon = Game::level->get_monster(coord);
  if (mon != nullptr) {
    if (player->can_see(*mon)) {
      print_monster(mon);
      return;

    } else if (player->can_sense_monsters()) {
      standout();
      print_monster(mon);
      standend();
      return;
    }
  }

  // Next prio: Items
  Item* item = Game::level->get_item(coord);
  if (item != nullptr) {
    print_item(item);
    return;
  }

  // Next prio: Floor
  print_color(coord.x, coord.y, Game::level->get_tile(coord));
}

void IO::print_tile_discovered(Coordinate const& coord) {

  // Next prio: Floor
  ::Tile::Type tile = Game::level->get_tile(coord);
  if (tile == ::Tile::Floor) {
    print_color(coord.x, coord.y, IO::Shadow);
  } else {
    print_color(coord.x, coord.y, tile);
  }

}

void IO::print_tile(Coordinate const& coord) {
  print_tile(coord.x, coord.y);
}

void IO::print_tile(int x, int y) {
  Coordinate coord(x, y);

  if (player->can_see(coord)) {
    print_tile_seen(coord);

  } else if (Game::level->is_discovered(coord)) {
    print_tile_discovered(coord);

  } else {
    print(x, y, IO::Shadow);
  }
}

chtype IO::colorize(chtype ch)
{
  if (!use_colors)
    return ch;

  // NOTE: COLOR_WHITE is black and COLOR_BLACK is white, because reasons

  switch (ch)
  {
    // Dungeon
    case IO::ClosedDoor:
    case IO::Wall: return ch | COLOR_PAIR(COLOR_WHITE) | A_BOLD;
    case IO::Trap: return ch | COLOR_PAIR(COLOR_RED);

    case IO::Floor:
    case IO::OpenDoor:
    case IO::Stairs: return ch | COLOR_PAIR(COLOR_YELLOW);

    // Items
    case IO::Gold: return ch | COLOR_PAIR(COLOR_YELLOW) | A_BOLD;

    // Monsters
    case 'b': return ch | COLOR_PAIR(COLOR_WHITE) | A_BOLD;
    case 'g': return ch | COLOR_PAIR(COLOR_YELLOW);
    case 'h': return ch | COLOR_PAIR(COLOR_GREEN);
    case 'i': return ch | COLOR_PAIR(COLOR_CYAN);
    case 'k': return ch | COLOR_PAIR(COLOR_YELLOW) | A_BOLD;
    case 'l': return ch | COLOR_PAIR(COLOR_GREEN) | A_BOLD;
    case 'n': return ch | COLOR_PAIR(COLOR_GREEN) | A_BOLD;
    case 'r': return ch | COLOR_PAIR(COLOR_RED);
    case 's': return ch | COLOR_PAIR(COLOR_GREEN);


    default: return ch | COLOR_PAIR(COLOR_BLACK);
  }
}

void IO::print_player_vision() {

  Coordinate const& player_pos = player->get_position();
  if (player_pos.x < 1 || player_pos.x >= NUMCOLS -1 ||
      player_pos.y < 1 || player_pos.y >= NUMLINES -1) {
    error("player_pos is too close to the edge");
  }

  if (player->is_blind()) {
    print_tile(player_pos);
  }

  int fov_range = 1;

  for (int y = player_pos.y -fov_range; y <= player_pos.y +fov_range; y++) {
    for (int x = player_pos.x -fov_range; x <= player_pos.x +fov_range; x++) {
      print_tile(x, y);
      Game::level->set_discovered(x, y);
    }
  }
}

void IO::print_level_layout() {
  /* take all the things we want to keep hidden out of the window */
  for (int y = 1; y < NUMLINES - 1; y++) {
    for (int x = 0; x < NUMCOLS; x++) {

      ::Tile::Type ch = Game::level->get_tile(x, y);
      switch (ch) {

        // Doors and stairs are always what they seem
        case ::Tile::OpenDoor: case ::Tile::ClosedDoor: case ::Tile::Stairs: break;

        // Check if walls are actually hidden doors
        case ::Tile::Wall: {
          if (!Game::level->is_real(x, y)) {
            ch = ::Tile::ClosedDoor;
            Game::level->set_tile(x, y, ::Tile::ClosedDoor);
            Game::level->set_real(x, y);
          }
        } break;

        // Floor can be traps. If it's not, we don't print it
        case ::Tile::Floor: {
          if (Game::level->is_real(x, y)) {
            ch = ::Tile::Floor;
            Game::level->set_discovered(x, y);
          } else {
            ch = ::Tile::Trap;
            Game::level->set_tile(x, y, ch);
            Game::level->set_discovered(x, y);
            Game::level->set_real(x, y);
          }
        } break;

        case ::Tile::Trap: break;
      }

      Monster* obj = Game::level->get_monster(x, y);
      if (obj == nullptr || !player->can_sense_monsters()) {
        print_color(x, y, ch);
      } else {
        print_monster(obj, IO::Attribute::Standout);
      }
    }
  }
}

void IO::refresh() {
  for (int x = 0; x < NUMCOLS -1; ++x) {
    for (int y = 1; y < NUMLINES -1; ++y) {
      print_tile(x, y);
    }
  }

  refresh_statusline();
  move(player->get_position().y, player->get_position().x);
  ::refresh();
}

void IO::refresh_statusline() {
  Coordinate original_position;
  getyx(stdscr, original_position.y, original_position.x);

  // Calculate width of hitpoint digits
  int hpwidth = 0;
  if (player->is_hurt()) {
    for (int temp = player->get_max_health(); temp > 0; temp /= 10, hpwidth++) {
      ;
    }
  }

  // Move to statusline and print
  mvprintw(NUMLINES -1, 0,
      "Depth: %dft.  Gold: %-5d  Hp: %*d(%*d)  Str: %2d(%d)  Arm: %-2d  Exp: %d/%d  %s",
      Game::current_level * 50, player->get_gold(), hpwidth, player->get_health(),
      hpwidth, player->get_max_health(), player->get_strength(),
      player->get_default_strength(), player->get_armor(), player->get_level(),
      player->get_experience(), player->get_hunger_state().c_str());

  clrtoeol();
  move(original_position.y, original_position.x);
}

void IO::repeat_last_messages() {
  wclear(extra_screen);
  wmove(extra_screen, 1, 0);
  for (string const& msg : last_messages) {
    waddch(extra_screen, '>');
    waddstr(extra_screen, msg.c_str());
    waddch(extra_screen, '\n');
  }
  show_extra_screen("Previous Messages: (press SPACE to return)");
}

string IO::read_string(WINDOW* win, string const* initial_string) {
  string return_value;

  Coordinate original_pos(static_cast<int>(message_buffer.size()), 0);
  wmove(win, original_pos.y, original_pos.x);

  if (initial_string != nullptr) {
    return_value = *initial_string;
    waddstr(win, initial_string->c_str());
  }

  // loop reading in the string, and put it in a temporary buffer
  for (;;) {

    wrefresh(win);
    int c = readchar(false);

    // Return on ESCAPE chars or ENTER
    if (c == '\n' || c == '\r' || c == -1 || c == KEY_ESCAPE) {
      break;

    // Remove char on BACKSPACE
    } else if (c == erasechar()) {
      if (!return_value.empty()) {
        return_value.pop_back();
        wmove(win, original_pos.y,
            original_pos.x + static_cast<int>(return_value.size()));
        wclrtoeol(win);
      }

    // Remove everything on killchar
    } else if (c == killchar()) {
      return_value.clear();
      wmove(win, original_pos.y, original_pos.x);
      wclrtoeol(win);

    // ~ gives home directory
    } else if (c == '~' && return_value.empty()) {
      return_value = os_homedir();
      if (return_value.size() > MAXINP) {
        return_value.resize(MAXINP);
      }
      waddstr(win, return_value.c_str());

    } else if (return_value.size() < MAXINP && (isprint(c) || c == ' ')) {
      return_value += static_cast<char>(c);
      waddch(win, static_cast<chtype>(c));
    }

#ifndef NDEBUG
    // Check that we haven't allowed something stupid
    int tmp_x, tmp_y;
    getyx(stdscr, tmp_y, tmp_x);
    Coordinate currpos(tmp_x, tmp_y);
    getmaxyx(stdscr, tmp_y, tmp_x);
    Coordinate maxpos(tmp_x, tmp_y);
    if (currpos.y < 0 || currpos.y >= maxpos.y) {
      error("Y is out of bounds");
    } else if (currpos.x < 0 || currpos.x >= maxpos.x) {
      error("X is out of bounds");
    }
#endif
  }

  // If empty, we use the initial string
  if (return_value.empty() && initial_string != nullptr) {
    return_value = *initial_string;
    waddstr(win, return_value.c_str());
  }

  clear_message();
  return return_value;
}

void IO::clear_message()
{
  move(0, 0);
  clrtoeol();
  message_buffer.clear();
}

void IO::show_extra_screen(string const& message)
{
  wmove(extra_screen, 0, 0);
  waddstr(extra_screen, message.c_str());
  touchwin(extra_screen);
  wmove(extra_screen, player->get_position().y, player->get_position().x);
  wrefresh(extra_screen);
  untouchwin(stdscr);

  wait_for_key(KEY_SPACE);

  clearok(curscr, true);
  touchwin(stdscr);
  clear_message();
}

void IO::message(string const& message, bool force_flush) {

  string const more_string = " --More--";
  size_t max_message = static_cast<size_t>(NUMCOLS) - more_string.size();

  // Pause when beginning on new line
  if (!message_buffer.empty() &&
      (message_buffer.size() + message.size() > max_message ||
      force_flush)) {
    message_buffer += more_string;

    mvaddstr(0, 0, message_buffer.c_str());
    clrtoeol();
    move(0, static_cast<int>(message_buffer.size()));
    ::refresh();
    int ch = getch();
    while (ch != KEY_SPACE && ch != '\n' && ch != '\r' && ch != KEY_ESCAPE) {
      ch = getch();
    }

    message_buffer.clear();
  }

  stringstream os;
  os << message_buffer;
  if (!message_buffer.empty()) {
    os << ". ";
  }

  if (message.size() > 1) {
    os
      << static_cast<char>(toupper(message.at(0)))
      << message.substr(1);
  } else if (message.size() > 0) {
    os << string(1, static_cast<char>(toupper(message.at(0))));
  }

  if (message.back() == '?') {
    os << " ";
  }

  message_buffer = os.str();
  mvaddstr(0, 0, message_buffer.c_str());

  last_messages.push_front(message_buffer);
  if (last_messages.size() > 20) {
    last_messages.pop_back();
  }

  clrtoeol();
}

void io_missile_motion(Item* item, int ydelta, int xdelta) {

  // Come fly with us ...
  item->set_position(player->get_position());

  for (;;) {

    Coordinate prev_pos = item->get_position();

    // Get the new position
    item->set_y(item->get_y() + ydelta);
    item->set_x(item->get_x() + xdelta);

    Coordinate new_pos = item->get_position();

    // Print old position
    if (player->can_see(prev_pos)) {
      Game::io->print_tile(prev_pos);
    }

    // See if we hit something
    Monster* monster = Game::level->get_monster(new_pos);
    Tile::Type tile = Game::level->get_tile(new_pos);
    if (monster != nullptr || tile == Tile::Wall) {
      break;
    }

    // Print new position
    if (player->can_see(new_pos)) {
      os_usleep(10000);
      Game::io->print_color(new_pos.x, new_pos.y, item->o_type);
      move(new_pos.y, new_pos.x);
      refresh();
    }
  }
}

