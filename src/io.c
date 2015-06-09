#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <assert.h>

#include "armor.h"
#include "misc.h"
#include "player.h"
#include "command.h"
#include "rogue.h"
#include "options.h"
#include "level.h"
#include "food.h"
#include "pack.h"

#include "io.h"

char huh[MAXSTR] = { '\0' };
WINDOW* hw = NULL;
int mpos = 0;

#define MAXMSG	(int)(NUMCOLS - sizeof " --More--")
static char msgbuf[2*MAXMSG+1];
static int newpos = 0;

static int
flushmsg(void)
{
  /* Nothing to show */
  if (msgbuf[0] == '\0')
    return ~KEY_ESCAPE;

  /* Save message in case player missed it */
  strcpy(huh, msgbuf);

  /* TODO: Remove mpos by replacing mpos = 0 with a clearmsg() */
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
    msgbuf[0] = (char) toupper(msgbuf[0]);
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
  static bool new_sentence = false;
  char const* separator = ". ";
  size_t separatorlen = strlen(separator);

  char buf[MAXSTR];
  vsprintf(buf, fmt, args);

  int msgsize = newpos + (int)strlen(buf);
  if (new_sentence)
    msgsize += separatorlen;

  if (msgsize >= MAXMSG)
    flushmsg();

  if (new_sentence && newpos != 0)
  {
    strcpy(&msgbuf[newpos], separator);
    newpos += separatorlen;
    buf[0] = (char) toupper(buf[0]);
    new_sentence = false;
  }

  strcpy(&msgbuf[newpos], buf);
  newpos = (int) strlen(msgbuf);
  new_sentence = end_of_command;
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
    char const* h = pw == NULL ? NULL : pw->pw_dir;

    if (h == NULL || *h == '\0' || !strcmp(h, "/"))
      h = getenv("HOME");
    if (h == NULL || !strcmp(h, "/"))
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
fail(char const* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  return 1;
}
#endif

int
clearmsg(void)
{
  move(0, 0);
  clrtoeol();
  mpos = 0;
  return ~KEY_ESCAPE;
}

__attribute__((__format__(__printf__, 1, 2)))
int
msg(char const* fmt, ...)
{
  va_list args;

  assert(fmt != NULL  && "Use clearmsg() instead of msg(NULL)");
  assert(*fmt != '\0' && "Use clearmsg() instead of msg(\"\")");

  /* otherwise add to the message */
  va_start(args, fmt);
  doadd(fmt, args, true);
  va_end(args);
  return ~KEY_ESCAPE;
}

__attribute__((__format__(__printf__, 1, 2)))
void
addmsg(char const* fmt, ...)
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
readchar(bool is_question)
{
  flushmsg();
  if (!is_question)
    move(player_y(), player_x());

  char ch = (char) getch();
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
status(void)
{
  int oy, ox;
  int hpwidth = 0;

  getyx(stdscr, oy, ox);

  if (player_is_hurt())
    for (int temp = player_get_max_health(); temp > 0; temp /= 10, hpwidth++)
      ;

  move(STATLINE, 0);
  printw("Level: %d  Gold: %-5d  Hp: %*d(%*d)  Str: %2d(%d)  Arm: %-2d  "
         "Exp: %d/%d  %s",
         level, purse, hpwidth, player_get_health(), hpwidth,
         player_get_max_health(), player_get_strength(), max_stats.s_str,
         player_get_armor(), player_get_level(), player_get_exp(),
         food_hunger_state());

  clrtoeol();
  move(oy, ox);
}

void
wait_for(int ch)
{
  switch (ch)
  {
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

void
show_win(const char *message)
{
  wmove(hw, 0, 0);
  waddstr(hw, message);
  touchwin(hw);
  wmove(hw, player_y(), player_x());
  wrefresh(hw);
  untouchwin(stdscr);

  wait_for(KEY_SPACE);

  clearok(curscr, true);
  touchwin(stdscr);
  clearmsg();
}

bool
wreadstr(WINDOW* win, char* dest)
{
  char buf[MAXSTR];
  int c = ~KEY_ESCAPE;
  int i = (int)strlen(dest);
  int oy, ox;

  assert(i >= 0);

  flushmsg();
  getyx(win, oy, ox);

  strucpy(buf, dest, i);
  waddstr(win, buf);

  /* loop reading in the string, and put it in a temporary buffer */
  while (c != KEY_ESCAPE)
  {
    wrefresh(win);
    c = readchar(true);

    if (c == '\n' || c == '\r' || c == -1)
      break;

    else if (c == erasechar() && i > 0)
    {
      i--;
      wmove(win, oy, ox + i);
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
      buf[i++] = (char)c;
      waddch(win, (chtype)c);
    }

#ifndef NDEBUG
    coord currpos;
    getyx(stdscr, currpos.y, currpos.x);
    coord maxpos;
    getmaxyx(stdscr, maxpos.y, maxpos.x);
    assert(currpos.y >= 0 && currpos.y < maxpos.y);
    assert(currpos.x >= 0 && currpos.x < maxpos.x);
#endif
  }

  buf[i] = '\0';
  if (i > 0) /* only change option if something has been typed */
    strucpy(dest, buf, (int) strlen(buf));
  else
    waddstr(win, dest);
  if (win == stdscr)
    mpos += i;

  wrefresh(win);
  return c == KEY_ESCAPE ? 1 : 0;
}

__attribute__((__format__(__printf__, 1, 2)))
void
fatal(char const* msg, ...)
{
  va_list args;

  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);

  abort();
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
