#include <curses.h>

#include "tiles.h"
#include "error_handling.h"

#include "io.h"

using namespace std;

static void curses_print(int x, int y, chtype ch, IO::Attribute attr) {
  if (y < 0 || y >= MAXLINES ||
      x < 0 || x >= MAXCOLS) {
    error("Attempted to print beyond screen! X: " +
          to_string(x) + ", Y: " + to_string(y) +
          "MAXLINES: " + to_string(MAXLINES) +
          "MAXCOLS: " + to_string(MAXCOLS));
  }
  switch (attr) {
    case IO::Attribute::None: break;
    case IO::Attribute::Standout: ch |= A_STANDOUT; break;
    case IO::Attribute::Red: ch |= COLOR_PAIR(COLOR_RED); break;
    case IO::Attribute::Blue: ch |= COLOR_PAIR(COLOR_BLUE); break;
  }
  mvaddch(y, x, ch);
}

template <>
void IO::print<char>(int x, int y, char ch, IO::Attribute attr) {
  curses_print(x, y, static_cast<chtype>(ch), attr);
}

template <>
void IO::print<unsigned int>(int x, int y, unsigned int ch, IO::Attribute attr) {
  curses_print(x, y, static_cast<chtype>(ch), attr);
}

template <>
void IO::print<IO::Tile>(int x, int y, IO::Tile ch, IO::Attribute attr) {
  curses_print(x, y, static_cast<chtype>(ch), attr);
}

template <>
void IO::print_color<char>(int x, int y, char ch, IO::Attribute attr) {
  curses_print(x, y, colorize(static_cast<chtype>(ch)), attr);
}

template <>
void IO::print_color<unsigned int>(int x, int y, unsigned int ch, IO::Attribute attr) {
  curses_print(x, y, colorize(static_cast<chtype>(ch)), attr);
}

template <>
void IO::print_color<int>(int x, int y, int ch, IO::Attribute attr) {
  curses_print(x, y, colorize(static_cast<chtype>(ch)), attr);
}

template <>
void IO::print_color<IO::Tile>(int x, int y, IO::Tile ch, IO::Attribute attr) {
  curses_print(x, y, colorize(static_cast<chtype>(ch)), attr);
}

template <>
void IO::print_color<::Tile::Type>(int x, int y, ::Tile::Type tile, IO::Attribute attr) {
  chtype ch;
  switch (tile) {
    case ::Tile::Floor:  ch = IO::Floor; break;
    case ::Tile::Wall:   ch = IO::Wall; break;
    case ::Tile::Door:   ch = IO::Door; break;
    case ::Tile::Trap:   ch = IO::Trap; break;
    case ::Tile::Stairs: ch = IO::Stairs; break;
  }
  curses_print(x, y, colorize(ch), attr);
}


