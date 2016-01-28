#include <curses.h>

#include "error_handling.h"

#include "io.h"

using namespace std;

static void curses_print(int x, int y, chtype ch) {
  if (y < 0 || y >= MAXLINES ||
      x < 0 || x >= MAXCOLS) {
    error("Attempted to print beyond screen! X: " +
          to_string(x) + ", Y: " + to_string(y) +
          "MAXLINES: " + to_string(MAXLINES) +
          "MAXCOLS: " + to_string(MAXCOLS));
  }
  mvaddch(y, x, ch);
}

template <>
void IO::print<char>(int x, int y, char ch) {
  curses_print(x, y, static_cast<chtype>(ch));
}

template <>
void IO::print<unsigned int>(int x, int y, unsigned int ch) {
  curses_print(x, y, static_cast<chtype>(ch));
}

template <>
void IO::print_color<char>(int x, int y, char ch) {
  curses_print(x, y, colorize(static_cast<chtype>(ch)));
}

template <>
void IO::print_color<unsigned int>(int x, int y, unsigned int ch) {
  curses_print(x, y, colorize(static_cast<chtype>(ch)));
}

template <>
void IO::print_color<int>(int x, int y, int ch) {
  curses_print(x, y, colorize(static_cast<chtype>(ch)));
}


