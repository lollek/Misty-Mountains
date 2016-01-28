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

void IO::print_tile(Coordinate const& coord) {
  print_tile(coord.x, coord.y);
}

void IO::print_tile(int x, int y) {
  // Highest prio: Monsters
  Monster* mon = Game::level->get_monster(x, y);
  if (mon != nullptr) {
    if (monster_seen_by_player(mon)) {
      print_color(x, y, mon->get_disguise());

    } else if (player->can_sense_monsters()) {
      standout();
      print_color(x, y, mon->get_disguise());
      standend();
    }

    return;
  }

  // Next prio: Items
  Item* item = Game::level->get_item(x, y);
  if (item != nullptr) {
    print_color(x, y, item->get_type());
    return;
  }

  // Next prio: Floor
  print_color(x, y, Game::level->get_ch(x, y));
}

WINDOW* hw = nullptr;

#define MAXMSG	static_cast<int>(NUMCOLS - sizeof " --More--")
static char msgbuf[2*MAXMSG+1];
static char last_msg[MAXSTR] = { '\0' };
static int newpos = 0;
static int mpos = 0;

static int
flushmsg(void)
{
  /* Nothing to show */
  if (msgbuf[0] == '\0')
    return ~KEY_ESCAPE;

  /* Save message in case player missed it */
  strcpy(last_msg, msgbuf);

  /* TODO: Remove mpos by replacing mpos = 0 with a io_msg_clear() */
  if (mpos)
  {
    look(false);
    mvaddstr(0, mpos, " --More--");
    refresh();

    int ch = getch();
    while (ch != KEY_SPACE && ch != '\n' && ch != '\r' && ch != KEY_ESCAPE)
      ch = getch();
  }

  /* All messages should start with uppercase, except ones that
   * start with a pack addressing character */
  if (islower(msgbuf[0]) && msgbuf[1] != ')')
    msgbuf[0] = static_cast<char>(toupper(msgbuf[0]));
  mvaddstr(0, 0, msgbuf);
  clrtoeol();
  mpos = newpos;
  newpos = 0;
  msgbuf[0] = '\0';
  refresh();
  return ~KEY_ESCAPE;
}

__attribute__((__format__(__printf__, 1, 0)))
static void
doadd(char const* fmt, va_list args, bool end_of_command)
{
  assert(fmt != nullptr  && "Use io_msg_clear() instead of io_msg(nullptr)");
  assert(*fmt != '\0' && "Use io_msg_clear() instead of io_msg(\"\")");

  static bool new_sentence = false;
  char const* separator = ". ";
  size_t separatorlen = strlen(separator);

  char buf[MAXSTR];
  vsprintf(buf, fmt, args);

  int msgsize = newpos + static_cast<int>(strlen(buf));
  if (new_sentence)
    msgsize += separatorlen;

  if (msgsize >= MAXMSG)
    flushmsg();

  if (new_sentence && newpos != 0)
  {
    strcpy(&msgbuf[newpos], separator);
    newpos += separatorlen;
    buf[0] = static_cast<char>(toupper(buf[0]));
    new_sentence = false;
  }

  strcpy(&msgbuf[newpos], buf);
  newpos = static_cast<int>(strlen(msgbuf));
  new_sentence = end_of_command;
}

void
io_msg_last(void)
{
  io_msg(last_msg);
}

char const*
get_homedir(void)
{
  static char homedir[PATH_MAX +1] = { '\0' };

  /* If we've already checked for homedir, we should know it by now */
  if (*homedir == '\0')
  {
    size_t len;
    struct passwd const* pw = getpwuid(getuid());
    char const* h = pw == nullptr ? nullptr : pw->pw_dir;

    if (h == nullptr || *h == '\0' || !strcmp(h, "/"))
      h = getenv("HOME");
    if (h == nullptr || !strcmp(h, "/"))
      h = "";

    /* PATH_MAX is not a hard limit,
     * so we need to check all sources carefully */

    if ((len = strlen(h)) < PATH_MAX && len > 0)
    {
      strcpy(homedir, h);
      if (homedir[len -1] != '/')
      {
        homedir[len] = '/';
        homedir[len +1] = '\0';
      }
    }
    else
      strcpy(homedir, "/");
  }
  return homedir;
}

chtype
colorize(const chtype ch)
{
  if (!use_colors)
    return ch;

  /* NOTE: COLOR_WHITE is black and COLOR_BLACK is white, because reasons */

  switch (ch)
  {
    /* Dungeon */
    case HWALL: case VWALL: return ch | COLOR_PAIR(COLOR_WHITE) | A_BOLD;
    case PASSAGE: case FLOOR: case STAIRS: return ch | COLOR_PAIR(COLOR_YELLOW);
    case TRAP: return ch | COLOR_PAIR(COLOR_RED);

    /* Items */
    case GOLD: return ch | COLOR_PAIR(COLOR_YELLOW) | A_BOLD;

    /* Monsters */
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

void
io_msg_clear(void)
{
  move(0, 0);
  clrtoeol();
  mpos = 0;
}

__attribute__((__format__(__printf__, 1, 2)))
void
io_msg(char const* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  doadd(fmt, args, true);
  va_end(args);
}

__attribute__((__format__(__printf__, 1, 2)))
void
io_msg_unsaved(char const* fmt, ...)
{
  char buf[MAXSTR];

  flushmsg();
  strcpy(buf, last_msg);

  va_list args;
  va_start(args, fmt);
  doadd(fmt, args, true);
  va_end(args);
  flushmsg();
  strcpy(last_msg, buf);
}

__attribute__((__format__(__printf__, 1, 2)))
void
io_msg_add(char const* fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  doadd(fmt, args, false);
  va_end(args);
}



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
io_refresh_statusline(void)
{
  int oy, ox;
  int hpwidth = 0;

  getyx(stdscr, oy, ox);

  if (player->is_hurt())
    for (int temp = player->get_max_health(); temp > 0; temp /= 10, hpwidth++)
      ;

  move(STATLINE, 0);
  printw("Level: %d  Gold: %-5d  Hp: %*d(%*d)  Str: %2d(%d)  Arm: %-2d  "
         "Exp: %d/%d  %s",
         Game::current_level, pack_gold, hpwidth, player->get_health(), hpwidth,
         player->get_max_health(), player->get_strength(), player->get_default_strength(),
         player->get_armor(), player->get_level(), player->get_experience(),
         food_hunger_state());

  clrtoeol();
  move(oy, ox);
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
show_win(const char *message)
{
  wmove(hw, 0, 0);
  waddstr(hw, message);
  touchwin(hw);
  wmove(hw, player->get_position().y, player->get_position().x);
  wrefresh(hw);
  untouchwin(stdscr);

  io_wait_for_key(KEY_SPACE);

  clearok(curscr, true);
  touchwin(stdscr);
  io_msg_clear();
}

bool
io_wreadstr(WINDOW* win, char* dest)
{
  char buf[MAXSTR];
  int c = ~KEY_ESCAPE;
  size_t i = strlen(dest);
  int oy, ox;

  flushmsg();
  getyx(win, oy, ox);

  strucpy(buf, dest, i);
  waddstr(win, buf);

  /* loop reading in the string, and put it in a temporary buffer */
  while (c != KEY_ESCAPE)
  {
    wrefresh(win);
    c = io_readchar(true);

    if (c == '\n' || c == '\r' || c == -1)
      break;

    else if (c == erasechar() && i > 0)
    {
      i--;
      wmove(win, oy, ox + static_cast<int>(i));
      wclrtoeol(win);
    }

    else if (c == killchar())
    {
      i = 0;
      wmove(win, oy, ox);
      wclrtoeol(win);
    }

    else if (c == '~' && i == 0)
    {
      strcpy(buf, get_homedir());
      waddstr(win, get_homedir());
      i += strlen(get_homedir());
    }

    else if (i < MAXINP && (isprint(c) || c == ' '))
    {
      buf[i++] = static_cast<char>(c);
      waddch(win, static_cast<chtype>(c));
    }

#ifndef NDEBUG
    int tmp_x, tmp_y;
    getyx(stdscr, tmp_y, tmp_x);
    Coordinate currpos(tmp_x, tmp_y);
    getmaxyx(stdscr, tmp_y, tmp_x);
    Coordinate maxpos(tmp_x, tmp_y);
    assert(currpos.y >= 0 && currpos.y < maxpos.y);
    assert(currpos.x >= 0 && currpos.x < maxpos.x);
#endif
  }

  buf[i] = '\0';
  if (i > 0) /* only change option if something has been typed */
    strucpy(dest, buf, strlen(buf));
  else
    waddstr(win, dest);
  if (win == stdscr)
    mpos += i;

  wrefresh(win);
  return c == KEY_ESCAPE ? 1 : 0;
}

chtype
io_attribute(enum attribute attribute)
{
  switch (attribute)
  {
    case ATTR_FIRE: return COLOR_PAIR(COLOR_RED);
    case ATTR_ICE:  return COLOR_PAIR(COLOR_BLUE);
    case ATTR_NONE: return 0;
  }
  assert(0 && "Unknown io_attribute");
  return 0;
}

chtype
io_tile(enum tile tile)
{
  switch (tile)
  {
    case TILE_BOLT_VERTICAL:  return  '|';
    case TILE_BOLT_DIAGUP:    return  '/';
    case TILE_BOLT_HORIZONTAL:return  '-';
    case TILE_BOLT_DIAGDOWN:  return '\\';

    case TILE_ERROR: return '?' | A_STANDOUT;
  }
  assert(0 && "Unknown io_tile");
  return 0;
}

void
io_missile_motion(Item* item, int ydelta, int xdelta)
{
  int ch;

  /* Come fly with us ... */
  item->set_pos(player->get_position());
  for (;;)
  {
    /* Erase the old one */
    if (item->get_pos() ==  player->get_position() &&
        player->can_see(item->get_pos()))
    {
      ch = Game::level->get_ch(item->get_pos());
      Game::io->print_color(item->get_x(), item->get_y(), ch);
    }

    /* Get the new position */
    item->set_y(item->get_y() + ydelta);
    item->set_x(item->get_x() + xdelta);
    if (step_ok(ch = Game::level->get_type(item->get_pos())) && ch != DOOR)
    {
      /* It hasn't hit anything yet, so display it if it alright. */
      if (player->can_see(item->get_pos()))
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

