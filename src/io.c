/*
 * Various input/output functions
 *
 * @(#)io.c	4.32 (Berkeley) 02/05/99
 */

#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <linux/limits.h>

#include "rogue.h"
#include "armor.h"

#include "io.h"

static void doadd(const char *fmt, va_list args); /* Add things to msgbuf */

const char *
get_homedir(void)
{
  static char homedir[PATH_MAX +1] = { '\0' };

  /* If we've already checked for homedir, we should know it by now */
  if (*homedir == '\0')
  {
    size_t len;
    struct passwd *pw = getpwuid(getuid());
    char *h = pw == NULL ? NULL : pw->pw_dir;

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
    case 'R': return ch | COLOR_PAIR(COLOR_RED);
    case 'S': return ch | COLOR_PAIR(COLOR_GREEN);


    default: return ch | COLOR_PAIR(COLOR_BLACK);
  }
}

int
msg(const char *fmt, ...)
{
  va_list args;

  /* if the string is "", just clear the line */
  if (*fmt == '\0')
  {
    move(0, 0);
    clrtoeol();
    mpos = 0;
    return ~KEY_ESCAPE;
  }

  /* otherwise add to the message and flush it out */
  va_start(args, fmt);
  doadd(fmt, args);
  va_end(args);
  return endmsg();
}

void
addmsg(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    doadd(fmt, args);
    va_end(args);
}

#define MAXMSG	(NUMCOLS - sizeof "--More--")
static char msgbuf[2*MAXMSG+1];
static int newpos = 0;

int
endmsg(void)
{
  char ch;

  /* Save message in case player missed it */
  strcpy(huh, msgbuf);
  /* TODO: Remove mpos by replacing mpos = 0 with a clearmsg() */
  if (mpos)
  {
    look(false);
    mvaddstr(0, mpos, "--More--");
    refresh();
    if (!msg_esc)
      wait_for(KEY_SPACE);
    else
    {
      while ((ch = readchar()) != KEY_SPACE)
        if (ch == KEY_ESCAPE)
        {
          msgbuf[0] = '\0';
          mpos = 0;
          newpos = 0;
          msgbuf[0] = '\0';
          return KEY_ESCAPE;
        }
    }
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

static void
doadd(const char *fmt, va_list args)
{
  char buf[MAXSTR];

  vsprintf(buf, fmt, args);
  if (strlen(buf) + newpos >= MAXMSG)
    endmsg();
  strcat(msgbuf, buf);
  newpos = (int) strlen(msgbuf);
}

bool
step_ok(int ch)
{
  if (ch == SHADOW || ch == HWALL || ch == VWALL)
    return false;
  return !isalpha(ch);
}

char
readchar(void)
{
  char ch = (char) getch();

  if (ch == 3)
  {
    quit(0);
    return(KEY_ESCAPE);
  }
  else
    return(ch);
}

void
status(void)
{
  int oy, ox;
  int hpwidth = 0;
  const char *state_name[] = { "", "Hungry", "Weak", "Faint" };

  getyx(stdscr, oy, ox);

  if (pstats.s_hpt != max_hp)
  {
    int temp;
    for (temp = max_hp; temp > 0; temp /= 10, hpwidth++)
      ;
  }

  move(STATLINE, 0);
  printw("Level: %d  Gold: %-5d  Hp: %*d(%*d)  Str: %2d(%d)  Arm: %-2d  "
         "Exp: %d/%d  %s",
         level, purse, hpwidth, pstats.s_hpt, hpwidth, max_hp, pstats.s_str,
         max_stats.s_str, armor_for_thing(&player), pstats.s_lvl, pstats.s_exp,
         state_name[hungry_state]);

  clrtoeol();
  move(oy, ox);
}

void
wait_for(int ch)
{
  if (ch == KEY_ENTER)
    ch = '\n';
  if (ch == '\n')
  {
    char c;
    while ((c = readchar()) != '\n' && c != '\r')
      ;
  }
  else
    while (readchar() != ch)
      ;
}

void
show_win(const char *message)
{

  wmove(hw, 0, 0);
  waddstr(hw, message);
  touchwin(hw);
  wmove(hw, hero.y, hero.x);
  wrefresh(hw);
  wait_for(KEY_SPACE);
  clearok(curscr, true);
  touchwin(stdscr);
}

bool
wreadstr(WINDOW *win, char *dest)
{
  char buf[MAXSTR];
  signed char c = ~KEY_ESCAPE;
  unsigned i = strlen(dest);
  int oy, ox;

  getyx(win, oy, ox);

  strucpy(buf, dest, i);
  waddstr(win, buf);

  /* loop reading in the string, and put it in a temporary buffer */
  while (c != KEY_ESCAPE)
  {
    wrefresh(win);
    c = readchar();

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
      buf[i++] = c;
      waddch(win, c);
    }
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

