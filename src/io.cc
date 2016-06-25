#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>

#include <assert.h>
#include <ctype.h>
#include <curses.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "command.h"
#include "error_handling.h"
#include "game.h"
#include "item/armor.h"
#include "item/food.h"
#include "level.h"
#include "misc.h"
#include "options.h"
#include "os.h"
#include "player.h"
#include "rogue.h"

#include "io.h"

using namespace std;

IO::IO() : last_messages(), message_buffer() {
  ESCDELAY = 0;
  initscr();  // Start up cursor package

  // Ncurses colors
  if (use_colors) {
    if (start_color() == ERR) {
      endwin();
      cerr << "Error: Failed to start colors. "
           << "Try restarting without colors enabled\n";
      Game::exit();
    }

    // Because ncurses has defined COLOR_BLACK to 0 and COLOR_WHITE to 7,
    // and then decided that init_pair cannot change number 0 (COLOR_BLACK)
    // I use COLOR_WHITE for black text and COLOR_BLACK for white text

    assume_default_colors(0, -1);  // Default is white text and any background
    init_pair(COLOR_RED, COLOR_RED, -1);
    init_pair(COLOR_GREEN, COLOR_GREEN, -1);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, -1);
    init_pair(COLOR_BLUE, COLOR_BLUE, -1);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, -1);
    init_pair(COLOR_CYAN, COLOR_CYAN, -1);
    init_pair(COLOR_WHITE, COLOR_BLACK, -1);
  }

  if (LINES < screen_height || COLS < screen_width) {
    endwin();
    cerr << "\nSorry, the screen must be at least " << screen_height << "x"
         << screen_width << "\n";
    Game::exit();
  }

  raw();     // Raw mode
  noecho();  // Echo off
}

IO::~IO() { endwin(); }

char IO::readchar(bool is_question) {
  if (is_question) {
    move(message_y,
         message_x + static_cast<int>(Game::io->message_buffer.size()));
  }

  char ch{static_cast<char>(getch())};
  switch (ch) {
    case 3: command_signal_quit(0); return KEY_ESCAPE;

    default: return ch;
  }
}

void IO::wait_for_key(int ch) {
  switch (ch) {
    case KEY_ENTER:
    case '\n':
      for (;;)
        if ((ch = readchar(true)) == '\n' || ch == '\r')
          return;

    default:
      for (;;)
        if (readchar(true) == ch)
          return;
  }
}

void IO::print(int x, int y, long unsigned int ch, IO::Attribute attr) {
  x += map_start_x;
  y += map_start_y;
  if (y < 0 || y >= screen_height || x < 0 || x >= screen_width) {
    error("Attempted to print beyond screen! X: " + to_string(x) + ", Y: " +
          to_string(y) + "MAXLINES: " + to_string(screen_height) + "MAXCOLS: " +
          to_string(screen_width));
  }
  switch (attr) {
    case IO::Attribute::None: break;
    case IO::Attribute::Standout: ch |= A_STANDOUT; break;
    case IO::Attribute::Red: ch |= COLOR_PAIR(COLOR_RED); break;
    case IO::Attribute::BoldRed: ch |= COLOR_PAIR(COLOR_RED) | A_BOLD; break;
    case IO::Attribute::Green: ch |= COLOR_PAIR(COLOR_GREEN); break;
    case IO::Attribute::BoldGreen:
      ch |= COLOR_PAIR(COLOR_GREEN) | A_BOLD;
      break;
    case IO::Attribute::Yellow: ch |= COLOR_PAIR(COLOR_YELLOW); break;
    case IO::Attribute::BoldYellow:
      ch |= COLOR_PAIR(COLOR_YELLOW) | A_BOLD;
      break;
    case IO::Attribute::Blue: ch |= COLOR_PAIR(COLOR_BLUE); break;
    case IO::Attribute::BoldBlue: ch |= COLOR_PAIR(COLOR_BLUE) | A_BOLD; break;
    case IO::Attribute::Magenta: ch |= COLOR_PAIR(COLOR_MAGENTA); break;
    case IO::Attribute::BoldMagenta:
      ch |= COLOR_PAIR(COLOR_MAGENTA) | A_BOLD;
      break;
    case IO::Attribute::Cyan: ch |= COLOR_PAIR(COLOR_CYAN); break;
    case IO::Attribute::BoldCyan: ch |= COLOR_PAIR(COLOR_CYAN) | A_BOLD; break;
    case IO::Attribute::Black: ch |= COLOR_PAIR(COLOR_WHITE); break;
    case IO::Attribute::BoldBlack:
      ch |= COLOR_PAIR(COLOR_WHITE) | A_BOLD;
      break;
    case IO::Attribute::White: ch |= COLOR_PAIR(COLOR_BLACK); break;
    case IO::Attribute::BoldWhite:
      ch |= COLOR_PAIR(COLOR_BLACK) | A_BOLD;
      break;
  }
  mvaddch(y, x, ch);
}

bool IO::print_monster(int x, int y, Monster* monster) {
  bool print_sensed_monster{false};

  if (!player->can_see(*monster)) {
    if (player->can_sense_monsters()) {
      print_sensed_monster = true;
    } else {
      return false;
    }
  }

  chtype symbol_to_print = static_cast<chtype>(monster->get_disguise());
  IO::Attribute attr = IO::None;

  if (monster->get_look() == monster->get_disguise()) {
    switch (monster->get_subtype()) {
      case Monster::Bat: attr = BoldBlack; break;
      case Monster::Goblin: attr = Yellow; break;
      case Monster::Kobold: attr = BoldYellow; break;
      case Monster::Orc: attr = BoldYellow; break;
      case Monster::Hobgoblin: attr = Green; break;
      case Monster::ZombieHuman: attr = BoldYellow; break;
      case Monster::SkeletonHuman: attr = BoldYellow; break;

      case Monster::Quasit: attr = Red; break;
      case Monster::Worg: attr = BoldBlack; break;
      case Monster::Ogre: attr = BoldYellow; break;
      case Monster::Yeti: attr = BoldWhite; break;
      case Monster::IceMonster: attr = BoldCyan; break;
      case Monster::Aquator: attr = Blue; break;
      case Monster::Troll: attr = BoldGreen; break;

      case Monster::ShadowDemon: attr = BoldBlack; break;
      case Monster::Erinyes: attr = BoldMagenta; break;
      case Monster::Nightmare: attr = BoldRed; break;
      case Monster::GenieDjinni: attr = BoldBlue; break;
      case Monster::GenieVizier: attr = Blue; break;
      case Monster::GenieEfreeti: attr = BoldRed; break;
      case Monster::GenieJanni: attr = BoldGreen; break;
      case Monster::GenieMarid: attr = BoldBlue; break;
      case Monster::GenieShaitan: attr = BoldYellow; break;

      case Monster::AdultRedDragon: attr = BoldRed; break;
      case Monster::Jabberwock: attr = BoldMagenta; break;

      case Monster::NMONSTERS: error("Bad monster subtype NMONSTERS");
    }
  }

  if (print_sensed_monster) {
    standout();
    print(x, y, symbol_to_print, attr);
    standend();
  } else {
    print(x, y, symbol_to_print, attr);
  }

  return true;
}

bool IO::print_item(int x, int y, Item* item) {
  chtype symbol_to_print = static_cast<chtype>(item->get_item_type());
  IO::Attribute attr = IO::None;
  if (symbol_to_print == IO::Gold) {
    attr = IO::BoldYellow;
  }
  print(x, y, symbol_to_print, attr);
  return true;
}

void IO::print_coordinate_seen(Coordinate const& coord) {
  Game::level->set_discovered(coord);

  // Next prio: Player
  if (player->get_position() == coord) {
    print(coord.x, coord.y, Tile::Player);
    return;
  }

  // Next prio: Monsters
  Monster* mon{Game::level->get_monster(coord)};
  if (mon != nullptr && print_monster(coord.x, coord.y, mon)) {
    return;
  }

  // Next prio: Items
  Item* item{Game::level->get_item(coord)};
  if (item != nullptr && print_item(coord.x, coord.y, item)) {
    return;
  }

  // Next prio: Floor
  print_tile(coord.x, coord.y, Game::level->get_tile(coord));
}

static IO::Attribute base_color(int x, int y) {
  if (Game::level->is_dark(x, y)) {
    return IO::BoldBlack;
  } else if (Game::current_level > 7 && Game::current_level < 14) {
    return IO::Cyan;
  } else if (Game::current_level > 13 && Game::current_level < 21) {
    return IO::Magenta;
  } else {
    return IO::Yellow;
  }
}

void IO::print_tile(int x, int y, ::Tile::Type tile) {
  chtype char_to_print{'?'};
  IO::Attribute attr = IO::None;
  switch (tile) {
    case ::Tile::Type::ClosedDoor: {
      char_to_print = IO::ClosedDoor;
      attr = IO::BoldBlack;
    } break;

    case ::Tile::Type::Wall: {
      char_to_print = IO::Wall;
      attr = IO::BoldBlack;
    } break;

    case ::Tile::Type::Trap: {
      char_to_print = IO::Trap;
      attr = IO::Red;
    } break;

    case ::Tile::Type::OpenDoor: {
      char_to_print = IO::OpenDoor;
      attr = base_color(x, y);
    } break;

    case ::Tile::Type::Floor: {
      char_to_print = IO::Floor;
      attr = base_color(x, y);
    } break;

    case ::Tile::Type::StairsDown: {
      char_to_print = IO::StairsDown;
      attr = base_color(x, y);
    } break;

    case ::Tile::Type::StairsUp: {
      char_to_print = IO::StairsUp;
      attr = base_color(x, y);
    } break;

    case ::Tile::Type::Shop: {
      char_to_print = IO::Shop;
      attr = IO::BoldBlue;
    } break;
  }
  print(x, y, char_to_print, attr);
}

void IO::print_coordinate(int x, int y) {
  Coordinate coord{x, y};

  // 1. What we see right now
  if (player->can_see(coord)) {
    print_coordinate_seen(coord);
    return;
  }
  // 2. What we sense
  if (player->can_sense_magic()) {
    Monster* monster{Game::level->get_monster(x, y)};
    if (monster != nullptr) {
      for (Item* item : monster->get_pack()) {
        if (item->is_magic()) {
          print(x, y, IO::Magic, IO::None);
          return;
        }
      }
    }

    Item* item{Game::level->get_item(x, y)};
    if (item != nullptr && item->is_magic()) {
      print(x, y, IO::Magic, IO::None);
      return;
    }
  }

  if (player->can_sense_monsters()) {
    Monster* monster{Game::level->get_monster(x, y)};
    if (monster != nullptr) {
      print(x, y, static_cast<chtype>(monster->get_look()), IO::Standout);
      return;
    }
  }

  // 3. What we remember
  if (Game::level->is_discovered(coord)) {
    ::Tile::Type tile{Game::level->get_tile(coord)};
    if (tile == ::Tile::Floor) {
      print(coord.x, coord.y, IO::Shadow);
    } else {
      print_tile(coord.x, coord.y, tile);
    }
    return;
  }

  // It's a good idea to draw black otherwise, to reduce artifacts
  print(x, y, IO::Shadow);
}

void IO::refresh() {
  refresh_statusline();
  for (int x{0}; x < map_width; ++x) {
    for (int y{0}; y < map_height; ++y) {
      print_coordinate(x, y);
    }
  }

  move_pointer(player->get_position().x, player->get_position().y);
  ::refresh();
}

void IO::refresh_statusline() {
  string race{Character::race_to_string(player->get_race())};
  race.at(0) = static_cast<char>(toupper(race.at(0)));

  // Calculate width of hitpoint digits
  stringstream ss;
  print_string(0, 0, "Name:  " + *Game::whoami);
  ss << "Race:  " << race << "\n"
     << "Class: Fighter\n"
     << "\n"
     << "Str:  " << player->get_strength() << " / "
     << player->get_default_strength() << "\n"
     << "Dex:  " << player->get_dexterity() << " / "
     << player->get_default_dexterity() << "\n"
     << "Con:  " << player->get_constitution() << " / "
     << player->get_default_constitution() << "\n"
     << "Wis:  " << player->get_wisdom() << " / "
     << player->get_default_wisdom() << "\n"
     << "Int:  " << player->get_intelligence() << " / "
     << player->get_default_intelligence() << "\n"
     << "Cha:  " << player->get_charisma() << " / "
     << player->get_default_charisma() << "\n"
     << "\n"
     << "Lvl:  " << setw(7) << player->get_level() << "\n"
     << "Exp:  " << setw(7) << player->get_experience() << "\n"
     << "Gold: " << setw(7) << player->get_gold() << "\n"
     << "Ac:   " << setw(7) << player->get_ac() << "\n"
     << "Hp:   " << player->get_health() << " / " << player->get_max_health()
     << "\n"
     << "Mp:    0 /  0\n"
     << "\n"
     << "Hunger: " << player->get_hunger_state() << "\n"
     << "Depth:  " << Game::current_level * 50 << "ft.\n";
  print_string(0, 1, ss.str());
}

void IO::repeat_last_messages() {
  move(1, 0);
  for (string const& msg : last_messages) {
    addch('>');
    addstr(msg.c_str());
    addch('\n');
  }

  mvaddstr(0, 0, "Previous Messages: (press any key to return)");
  getch();
  clear();
}

string IO::read_string(string const* initial_string, bool question) {
  string return_value;

  Coordinate original_pos{static_cast<int>(message_buffer.size()), 0};
  if (question) {
    move(original_pos.y, original_pos.x);
  }

  if (initial_string != nullptr) {
    return_value = *initial_string;
    addstr(initial_string->c_str());
  }

  // loop reading in the string, and put it in a temporary buffer
  for (;;) {
    ::refresh();
    int const c{readchar(false)};

    // Return on ESCAPE chars or ENTER
    if (c == '\n' || c == '\r' || c == -1 || c == KEY_ESCAPE) {
      break;

      // Remove char on BACKSPACE
    } else if (c == erasechar()) {
      if (!return_value.empty()) {
        return_value.pop_back();
        int curry;
        int currx;
        getyx(stdscr, curry, currx);
        move(curry, currx - 1);
        clrtoeol();
      }

      // Remove everything on killchar
    } else if (c == killchar()) {
      return_value.clear();
      move(original_pos.y, original_pos.x);
      clrtoeol();

      // ~ gives home directory
    } else if (c == '~' && return_value.empty()) {
      return_value = os_homedir();
      if (return_value.size() > max_input) {
        return_value.resize(max_input);
      }
      addstr(return_value.c_str());

    } else if (return_value.size() < max_input && (isprint(c) || c == ' ')) {
      return_value += static_cast<char>(c);
      addch(static_cast<chtype>(c));
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
    addstr(return_value.c_str());
  }

  clear_message();
  return return_value;
}

void IO::clear_message() {
  move(message_y, message_x);
  clrtoeol();
  message_buffer.clear();
}

void IO::message(string const& message, bool force_flush) {
  string const more_string{" --More--"};
  size_t const max_message{static_cast<size_t>(screen_width) -
                           more_string.size()};

  // Pause when beginning on new line
  if (!message_buffer.empty() &&
      (message_buffer.size() + message.size() > max_message || force_flush)) {
    message_buffer += more_string;

    mvaddstr(message_y, message_x, message_buffer.c_str());
    move(message_y, message_x + static_cast<int>(message_buffer.size()));
    clrtoeol();
    ::refresh();
    int ch{getch()};
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
    os << static_cast<char>(toupper(message.at(0))) << message.substr(1);
  } else if (message.size() > 0) {
    os << string(1, static_cast<char>(toupper(message.at(0))));
  }

  if (message.back() == '?') {
    os << " ";
  }

  message_buffer = os.str();
  mvaddstr(message_y, message_x, message_buffer.c_str());

  last_messages.push_front(message_buffer);
  if (last_messages.size() > 20) {
    last_messages.pop_back();
  }

  clrtoeol();
}

void IO::missile_motion(Item* item, int ydelta, int xdelta) {
  // Come fly with us ...
  item->set_position(player->get_position());

  for (;;) {
    Coordinate prev_pos = item->get_position();

    // Get the new position
    item->set_y(item->get_y() + ydelta);
    item->set_x(item->get_x() + xdelta);

    Coordinate new_pos{item->get_position()};

    // Print old position
    if (player->can_see(prev_pos)) {
      Game::io->print_coordinate(prev_pos);
    }

    // See if we hit something
    Monster const* monster{Game::level->get_monster(new_pos)};
    ::Tile::Type const tile{Game::level->get_tile(new_pos)};
    if (monster != nullptr || tile == ::Tile::Wall) {
      break;
    }

    // Print new position
    if (player->can_see(new_pos)) {
      os_usleep(10000);
      Game::io->print(new_pos.x, new_pos.y, static_cast<chtype>(item->o_type),
                      IO::None);
      move_pointer(new_pos.x, new_pos.y);
      ::refresh();
    }
  }
}

void IO::force_redraw() {
  clearok(curscr, true);
  ::wrefresh(curscr);
}

void IO::move_pointer(int x, int y) { move(map_start_y + y, map_start_x + x); }

void IO::clear_screen() { clear(); }

void IO::print_char(int x, int y, char sym) { mvaddch(y, x, sym); }

void IO::print_string(int x, int y, string const& str) {
  mvaddstr(y, x, str.c_str());
}

void IO::print_string(string const& str) { addstr(str.c_str()); }

void IO::stop_curses() { endwin(); }

void IO::resume_curses() {
  noecho();
  raw();
  clearok(stdscr, true);
}

void IO::character_creation() {
  vector<int> stats;
  Character::Race race;

  stringstream ss;

  ss << "Name :\n"
     << "Race :\n"
     << "Class:\n"
     << "\n"
     << "STR  :\n"
     << "DEX  :\n"
     << "CON  :\n"
     << "INT  :\n"
     << "WIS  :\n"
     << "CHA  :\n";

  print_string(0, 2, ss.str());
  ss.str("");

  // Roll stats
  bool reroll_stats = true;
  for (;;) {
    if (reroll_stats) {
      stats.clear();

      while (stats.size() < 6) {
        vector<int> rolls{roll(1, 6), roll(1, 6), roll(1, 6), roll(1, 6)};
        sort(rolls.begin(), rolls.end());
        stats.push_back(accumulate(rolls.begin() + 1, rolls.end(), 0));
      }

      reroll_stats = false;
    }

    ss << "STR  :" << setw(3) << stats.at(0) << "\n"
       << "DEX  :" << setw(3) << stats.at(1) << "\n"
       << "CON  :" << setw(3) << stats.at(2) << "\n"
       << "INT  :" << setw(3) << stats.at(3) << "\n"
       << "WIS  :" << setw(3) << stats.at(4) << "\n"
       << "CHA  :" << setw(3) << stats.at(5) << "\n";
    print_string(0, 6, ss.str());
    print_string(0, 0, "SPACE to reroll stats. 'y' to accept stats");
    ss.str("");

    char ch{readchar(false)};
    if (ch == 'y') {
      break;
    } else if (ch == KEY_SPACE) {
      reroll_stats = true;
    }
  }

  // Select Race
  for (;;) {
    ss << "Available races:\n"
       << "(H)uman";
    print_string(0, 15, ss.str());
    ss.str("");
    print_string(0, 0, "Select race");
    clrtoeol();

    switch (readchar(false)) {
      case 'h':
      case 'H': race = Character::Human; break;
      default: continue;
    }

    string race_str = Character::race_to_string(race);
    race_str.at(0) = static_cast<char>(toupper(race_str.at(0)));
    print_string(0, 3, "Race : " + race_str + "\n");

    print_string(0, 0, "SPACE to repick race, 'y' to accept race");
    char ch{readchar(false)};
    if (ch == 'y') {
      break;
    }
  }

  player = new class Player(stats, race);
  clear_screen();
}
