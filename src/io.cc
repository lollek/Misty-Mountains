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
#include "pack.h"
#include "player.h"
#include "rogue.h"

#include "io.h"

using namespace std;

// TODO: Remove these
static int flushmsg(void);

IO::IO() : last_message(), message_buffer(), extra_screen(nullptr) {
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


void IO::print_monster(Monster* monster, IO::Attribute attr) {
  int symbol_to_print = monster->get_disguise();
  Coordinate const& coord = monster->get_position();
  print_color(coord.x, coord.y, symbol_to_print, attr);
}

void IO::print_item(Item* item) {
  int symbol_to_print = item->get_type();
  Coordinate const& coord = item->get_position();
  print_color(coord.x, coord.y, symbol_to_print);
}

void IO::print_tile(Coordinate const& coord) {
  print_tile(coord.x, coord.y);
}

void IO::print_tile(int x, int y) {
  Coordinate coord(x, y);

  // Next prio: Player
  if (player->get_position() == coord) {
    print_color(x, y, player->get_type());
    return;
  }

  // Next prio: Monsters
  Monster* mon = Game::level->get_monster(x, y);
  if (mon != nullptr) {
    if (monster_seen_by_player(mon)) {
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
  Item* item = Game::level->get_item(x, y);
  if (item != nullptr) {
    print_item(item);
    return;
  }

  // Next prio: Floor
  print_color(x, y, Game::level->get_ch(x, y));
}

void IO::hide_tile(Coordinate const& coord) {
  hide_tile(coord.x, coord.y);
}

void IO::hide_tile(int x, int y) {
  print(x, y, SHADOW);
}

chtype IO::colorize(chtype ch)
{
  if (!use_colors)
    return ch;

  // NOTE: COLOR_WHITE is black and COLOR_BLACK is white, because reasons

  switch (ch)
  {
    // Dungeon
    case HWALL: case VWALL: return ch | COLOR_PAIR(COLOR_WHITE) | A_BOLD;
    case PASSAGE: case FLOOR: case STAIRS: return ch | COLOR_PAIR(COLOR_YELLOW);
    case TRAP: return ch | COLOR_PAIR(COLOR_RED);

    // Items
    case GOLD: return ch | COLOR_PAIR(COLOR_YELLOW) | A_BOLD;

    // Monsters
    case 'B': return ch | COLOR_PAIR(COLOR_WHITE) | A_BOLD;
    case 'E': return ch | COLOR_PAIR(COLOR_MAGENTA);
    case 'H': return ch | COLOR_PAIR(COLOR_GREEN);
    case 'I': return ch | COLOR_PAIR(COLOR_CYAN);
    case 'K': return ch | COLOR_PAIR(COLOR_YELLOW) | A_BOLD;
    case 'L': return ch | COLOR_PAIR(COLOR_GREEN) | A_BOLD;
    case 'N': return ch | COLOR_PAIR(COLOR_GREEN) | A_BOLD;
    case 'R': return ch | COLOR_PAIR(COLOR_RED);
    case 'S': return ch | COLOR_PAIR(COLOR_GREEN);


    default: return ch | COLOR_PAIR(COLOR_BLACK);
  }
}

void IO::print_room_dark(room const* room) {
  hide_room(room);
  print_player_vision();
}

void IO::print_player_vision() {

  Coordinate const& player_pos = player->get_position();
  if (player_pos.x < 1 || player_pos.x >= NUMCOLS -1 ||
      player_pos.y < 1 || player_pos.y >= NUMLINES -1) {
    error("player_pos is too close to the edge");
  }

  for (int y = player_pos.y -1; y <= player_pos.y +1; y++) {
    for (int x = player_pos.x -1; x <= player_pos.x +1; x++) {

      // Ignore ' ' (shadow)
      char xy_ch = Game::level->get_ch(x, y);
      if (xy_ch == SHADOW) {
        continue;
      }

      // Make sure we don't look though walls
      bool xy_is_passage = Game::level->is_passage(x, y);
      bool player_in_passage = Game::level->is_passage(player->get_position());
      char player_ch = Game::level->get_ch(player->get_position());
      if (player_ch != DOOR && xy_ch != DOOR &&
          player_in_passage != xy_is_passage) {
        continue;
      }

      // Make sure we cannot see diagonals in passages, since that can be a bit
      // like cheating
      if ((xy_is_passage || xy_ch == DOOR) &&
          (player_in_passage || player_ch == DOOR) &&
          player->get_position().x != x && player->get_position().y != y
          && !step_ok(Game::level->get_ch(player->get_position().x, y))
          && !step_ok(Game::level->get_ch(x, player->get_position().y))) {
        continue;
      }

      if (player->is_blind() &&
          (y != player->get_position().y || x != player->get_position().x)) {
        continue;
      }

      print_tile(x, y);
      Game::level->set_discovered(x, y);
    }
  }
}

void IO::print_room_light(room const* room) {
  for (int y = room->r_pos.y; y < room->r_max.y + room->r_pos.y; y++) {
    for (int x = room->r_pos.x; x < room->r_max.x + room->r_pos.x; x++) {
      print_tile(x, y);
    }
  }

  // Do a player-vision-print as well, so we can see into any nearby passage
  print_player_vision();
}

void IO::print_room(room const* room) {
  if (player->is_blind()) {
    return;
  } else if (room->r_flags & ISDARK) {
    print_room_dark(room);
  } else if (room->r_flags & (ISGONE|ISMAZE)) {
    print_player_vision();
  } else {
    print_room_light(room);
  }
}

void IO::hide_room(room const* room) {
  for (int y = room->r_pos.y; y < room->r_max.y + room->r_pos.y; y++) {
    for (int x = room->r_pos.x; x < room->r_max.x + room->r_pos.x; x++) {
      switch (Game::level->get_ch(x, y)) {

        // Things to NOT hide:
        case HWALL: case VWALL: case DOOR: case STAIRS: case TRAP: {
          break;
        }

        default: {
          hide_tile(x, y);
        } break;
      }
    }
  }
}

void IO::print_level_layout() {
  /* take all the things we want to keep hidden out of the window */
  for (int y = 1; y < NUMLINES - 1; y++) {
    for (int x = 0; x < NUMCOLS; x++) {

      char ch = Game::level->get_ch(x, y);
      switch (ch) {

        // Doors and stairs are always what they seem
        case DOOR: case STAIRS: break;

        // Check if walls are actually hidden doors
        case HWALL: case VWALL: {
          if (!Game::level->is_real(x, y)) {
            ch = DOOR;
            Game::level->set_ch(x, y, DOOR);
            Game::level->set_real(x, y);
          }
        } break;

        // Floor can be traps. If it's not, we don't print it
        case FLOOR: {
          if (Game::level->is_real(x, y)) {
            ch = SHADOW;
          } else {
            ch = TRAP;
            Game::level->set_ch(x, y, ch);
            Game::level->set_discovered(x, y);
            Game::level->set_real(x, y);
          }
        } break;

        // Shadow can be a hidden passage
        case SHADOW: {
          if (Game::level->is_real(x, y)) {
            break;
          }
          ch = PASSAGE;
          Game::level->set_ch(x, y, ch);
          Game::level->set_real(x, y);
        } /* FALLTHROUGH */

        [[clang::fallthrough]];
        // Seems like many things can be a passage?
        case PASSAGE: {
pass:
          if (!Game::level->is_real(x, y)) {
            Game::level->set_ch(x, y, PASSAGE);
          }
          ch = PASSAGE;
          Game::level->set_discovered(x, y);
          Game::level->set_real(x, y);
        } break;

        default: {
          if (Game::level->is_passage(x, y)) {
            goto pass;
          }
          ch = SHADOW;
        } break;
      }

      if (ch != SHADOW) {
        Monster* obj = Game::level->get_monster(x, y);
        if (obj == nullptr || !player->can_sense_monsters()) {
          print_color(x, y, ch);
        } else {
          print_monster(obj, IO::Attribute::Standout);
        }
      }
    }
  }
}

void IO::refresh() {
  print_room(player->get_room());

  refresh_statusline();
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
      "Level: %d  Gold: %-5d  Hp: %*d(%*d)  Str: %2d(%d)  Arm: %-2d  Exp: %d/%d  %s",
      Game::current_level, pack_gold, hpwidth, player->get_health(), hpwidth,
      player->get_max_health(), player->get_strength(), player->get_default_strength(),
      player->get_armor(), player->get_level(), player->get_experience(),
      food_hunger_state());

  clrtoeol();
  move(original_position.y, original_position.x);
}

void IO::repeat_last_message() {
  message(last_message);
}

string IO::read_string(WINDOW* win, string const* initial_string) {
  string return_value;

  flushmsg();
  Coordinate original_pos;
  getyx(win, original_pos.y, original_pos.x);

  if (initial_string != nullptr) {
    return_value = *initial_string;
    waddstr(win, initial_string->c_str());
  }

  // loop reading in the string, and put it in a temporary buffer
  for (;;) {

    wrefresh(win);
    int c = io_readchar(true);

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

  wrefresh(win);
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

  io_wait_for_key(KEY_SPACE);

  clearok(curscr, true);
  touchwin(stdscr);
  clear_message();
}

void IO::message(string const& message, bool new_sentence) {

  size_t max_message = static_cast<size_t>(NUMCOLS) - string(" --More--").size();

   if (message_buffer.size() + message.size() > max_message) {
    flushmsg();

   } else if (new_sentence && !message_buffer.empty()) {
     message_buffer += ". ";
   }

   if (message.size() > 1) {
     stringstream os;
     os
       << message_buffer
       << static_cast<char>(toupper(message.at(0)))
       << message.substr(1);
     message_buffer = os.str();

   } else if (message.size() > 0) {
     message_buffer += string(1, static_cast<char>(toupper(message.at(0))));
   }
}












static int newpos = 0;
static int mpos = 0;

static int
flushmsg(void)
{
  /* Nothing to show */
  if (Game::io->message_buffer.empty()) {
    return ~KEY_ESCAPE;
  }

  /* Save message in case player missed it */
  Game::io->last_message = Game::io->message_buffer;

  /* TODO: Remove mpos by replacing mpos = 0 with a io_msg_clear() */
  if (mpos)
  {
    mvaddstr(0, mpos, " --More--");
    refresh();

    int ch = getch();
    while (ch != KEY_SPACE && ch != '\n' && ch != '\r' && ch != KEY_ESCAPE)
      ch = getch();
  }

  /* All messages should start with uppercase, except ones that
   * start with a pack addressing character */
  if (islower(Game::io->message_buffer.at(0)) && Game::io->message_buffer.at(1) != ')')
    Game::io->message_buffer.at(0) = static_cast<char>(toupper(Game::io->message_buffer.at(0)));
  mvaddstr(0, 0, Game::io->message_buffer.c_str());
  clrtoeol();
  mpos = newpos;
  newpos = 0;
  Game::io->message_buffer.clear();
  refresh();
  return ~KEY_ESCAPE;
}

#ifndef NDEBUG
__attribute__((__format__(__printf__, 1, 2)))
bool
io_fail(char const* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  return 1;
}

__attribute__((__format__(__printf__, 1, 2)))
void
io_debug(char const* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
__attribute__((__format__(__printf__, 1, 2)))
void
io_debug_fatal(char const* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  assert(0);
}
#pragma clang diagnostic pop

#endif

bool
step_ok(int ch)
{
  if (ch == SHADOW || ch == HWALL || ch == VWALL)
    return false;
  return !isalpha(ch);
}

char
io_readchar(bool is_question)
{
  flushmsg();
  if (!is_question)
    move(player->get_position().y, player->get_position().x);

  char ch = static_cast<char>(getch());
  switch (ch)
  {
    case 3:
      command_signal_quit(0);
      return KEY_ESCAPE;

    default:
      return ch;
  }
}

void
io_wait_for_key(int ch)
{
  switch (ch)
  {
    case KEY_ENTER: case '\n':
      for (;;)
        if ((ch = io_readchar(true)) == '\n' || ch == '\r')
          return;

    default:
      for (;;)
        if (io_readchar(true) == ch)
          return;
  }
}

void
io_missile_motion(Item* item, int ydelta, int xdelta)
{
  int ch;

  /* Come fly with us ... */
  item->set_position(player->get_position());
  for (;;)
  {
    /* Erase the old one */
    if (item->get_position() ==  player->get_position() &&
        player->can_see(item->get_position()))
    {
      ch = Game::level->get_ch(item->get_position());
      Game::io->print_color(item->get_x(), item->get_y(), ch);
    }

    /* Get the new position */
    item->set_y(item->get_y() + ydelta);
    item->set_x(item->get_x() + xdelta);
    if (step_ok(ch = Game::level->get_type(item->get_position())) && ch != DOOR)
    {
      /* It hasn't hit anything yet, so display it if it alright. */
      if (player->can_see(item->get_position()))
      {
        os_usleep(10000);
        Game::io->print_color(item->get_x(), item->get_y(), item->o_type);
        refresh();
      }
      continue;
    }
    break;
  }
}

