#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <string>

using namespace std;

#include "error_handling.h"
#include "game.h"
#include "armor.h"
#include "colors.h"
#include "daemons.h"
#include "io.h"
#include "level.h"
#include "monster.h"
#include "move.h"
#include "options.h"
#include "os.h"
#include "pack.h"
#include "player.h"
#include "rings.h"
#include "rogue.h"
#include "level_rooms.h"

#include "misc.h"

int
roll(int number, int sides)
{
  int dtotal = 0;

  while (number--)
    dtotal += os_rand_range(sides) + 1;
  return dtotal;
}

string
vowelstr(string const& str)
{
  switch (str[0])
  {
    case 'a': case 'A':
    case 'e': case 'E':
    case 'i': case 'I':
    case 'o': case 'O':
    case 'u': case 'U':
      return "n";
    default:
      return "";
  }
}

Coordinate const*
get_dir(void)
{
  static Coordinate delta;

  char const* prompt = "which direction? ";
  io_msg(prompt);

  bool gotit;
  do
  {
    gotit = true;
    switch (dir_ch = io_readchar(false))
    {
      case 'h': case 'H': delta.y =  0; delta.x = -1; break;
      case 'j': case 'J': delta.y =  1; delta.x =  0; break;
      case 'k': case 'K': delta.y = -1; delta.x =  0; break;
      case 'l': case 'L': delta.y =  0; delta.x =  1; break;
      case 'y': case 'Y': delta.y = -1; delta.x = -1; break;
      case 'u': case 'U': delta.y = -1; delta.x =  1; break;
      case 'b': case 'B': delta.y =  1; delta.x = -1; break;
      case 'n': case 'N': delta.y =  1; delta.x =  1; break;

      case KEY_ESCAPE:
        io_msg_clear();
        return nullptr;

      default:
        io_msg_clear();
        io_msg(prompt);
        gotit = false;
        break;
    }
  } while (!gotit);

  if (isupper(dir_ch))
    dir_ch = static_cast<char>(tolower(dir_ch));

  if (player->is_confused() && os_rand_range(5) == 0)
    do
    {
      delta.y = os_rand_range(3) - 1;
      delta.x = os_rand_range(3) - 1;
    } while (delta.y == 0 && delta.x == 0);

  io_msg_clear();
  return &delta;
}

int
sign(int nm)
{
  if (nm < 0)
    return -1;
  else
    return (nm > 0);
}

int
spread(int nm)
{
  return nm - nm / 20 + os_rand_range(nm / 10);
}

char
rnd_thing(void)
{
  int i = os_rand_range(Game::current_level >= Game::amulet_min_level ? 10 : 9);
  switch (i)
  {
    case 0: return POTION;
    case 1: return SCROLL;
    case 2: return RING;
    case 3: return STICK;
    case 4: return FOOD;
    case 5: return WEAPON;
    case 6: return ARMOR;
    case 7: return STAIRS;
    case 8: return GOLD;
    case 9:
      if (Game::current_level < Game::amulet_min_level)
        io_debug("rnd_thing: Amulet spawned at a too low level", 0);
      return AMULET;

    default:
      io_debug("rnd_thing got %d, expected value between 0 and 9", 0);
      return GOLD;
  }
}

void
strucpy(char* dst, char const* src, size_t len)
{
  if (len > MAXINP)
    len = MAXINP;
  while (len--)
  {
    if (isprint(*src) || *src == ' ')
      *dst++ = *src;
    src++;
  }
  *dst = '\0';
}

bool
diag_ok(Coordinate const* sp, Coordinate const* ep)
{
  if (ep->x < 0 || ep->x >= NUMCOLS || ep->y <= 0 || ep->y >= NUMLINES - 1)
    return false;
  if (ep->x == sp->x || ep->y == sp->y)
    return true;
  return (step_ok(Game::level->get_ch(sp->x, ep->y))
             && step_ok(Game::level->get_ch(ep->x, sp->y)));
}

int
dist(int y1, int x1, int y2, int x2)
{
    return ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

char
floor_ch(void)
{
  return (player->get_room()->r_flags & ISGONE)
    ? PASSAGE : FLOOR;
}

char
floor_at(void)
{
  char ch = Game::level->get_ch(player->get_position());
  return ch == FLOOR ? floor_ch() : ch;
}

bool
fallpos(Coordinate const* pos, Coordinate* newpos)
{
  int cnt = 0;
  for (int y = pos->y - 1; y <= pos->y + 1; y++)
    for (int x = pos->x - 1; x <= pos->x + 1; x++)
    {
      /*
       * check to make certain the spot is empty, if it is,
       * put the object there, set it in the level list
       * and re-draw the room if he can see it
       */
      if (y == player->get_position().y && x == player->get_position().x)
        continue;

      int ch = Game::level->get_ch(x, y);
      if ((ch == FLOOR || ch == PASSAGE) && os_rand_range(++cnt) == 0)
      {
        newpos->y = y;
        newpos->x = x;
      }
    }
  return cnt != 0;
}


