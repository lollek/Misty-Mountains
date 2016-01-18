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

/* Return the character appropriate for this space, taking into
 * account whether or not the player is tripping */
static int
trip_ch(int y, int x, int ch)
{
  if (player_is_hallucinating())
    switch (ch)
    {
      case FLOOR: case SHADOW: case PASSAGE: case HWALL: case VWALL: case DOOR:
      case TRAP:
        break;
      default:
        if (y != Game::level->get_stairs_y() ||
            x != Game::level->get_stairs_x() ||
            !player->has_seen_stairs())
          return rnd_thing();
    }
  return ch;
}

int
roll(int number, int sides)
{
  int dtotal = 0;

  while (number--)
    dtotal += os_rand_range(sides) + 1;
  return dtotal;
}

void
look(bool wakeup)
{
  Coordinate const* player_pos = player_get_pos();
  char const player_ch = Game::level->get_ch(*player_pos);
  char const player_flags = Game::level->get_flags(*player_pos);

  if (move_pos_prev == *player_pos)
  {
    erase_lamp(&move_pos_prev, room_prev);
    move_pos_prev = *player_pos;
    room_prev = player_get_room();
  }

  int sumhero = 0;
  int diffhero = 0;
  if (door_stop && !firstmove && running)
  {
    sumhero = player_pos->y + player_pos->x;
    diffhero = player_pos->y - player_pos->x;
  }

  int passcount = 0;
  for (int y = player_pos->y - 1; y <= player_pos->y + 1; y++)
  {
    if (y <= 0 || y >= NUMLINES -1)
      continue;

    for (int x = player_pos->x -1; x <= player_pos->x + 1; x++)
    {
      if (x < 0 || x >= NUMCOLS)
        continue;

      if (!player_is_blind()
          && y == player_pos->y && x == player_pos->x)
        continue;

      char xy_ch = Game::level->get_ch(x, y);
      if (xy_ch == SHADOW)  /* nothing need be done with a ' ' */
        continue;

      char const xy_flags = Game::level->get_flags(x, y);
      if (player_ch != DOOR
          && xy_ch != DOOR
          && (player_flags & F_PASS) != (xy_flags & F_PASS))
        continue;

      if (((xy_flags & F_PASS) || xy_ch == DOOR)
          && ((player_flags & F_PASS) || player_ch == DOOR))
      {
        if (player_pos->x != x && player_pos->y != y
            && !step_ok(Game::level->get_ch(player_pos->x, y))
            && !step_ok(Game::level->get_ch(x, player_pos->y)))
          continue;
      }

      Monster* monster = Game::level->get_monster(x, y);
      if (monster == nullptr)
        xy_ch = static_cast<char>(trip_ch(y, x, xy_ch));
      else
      {
        if (player_can_sense_monsters() && monster_is_invisible(monster))
        {
          if (door_stop && !firstmove)
            running = false;
          continue;
        }
        else
        {
          if (wakeup)
            monster_notice_player(y, x);
          if (monster_seen_by_player(monster))
          {
            xy_ch = player_is_hallucinating()
              ? static_cast<char>(os_rand_range(26) + 'A')
              : monster->t_disguise;
          }
        }
      }

      if (player_is_blind() && (y != player_pos->y || x != player_pos->x))
        continue;

      move(y, x);

      if (monster != nullptr || xy_ch != incch())
        addcch(static_cast<chtype>(xy_ch));

      if (door_stop && !firstmove && running)
      {
        if (   (runch == 'h' && x == player_pos->x + 1)
            || (runch == 'j' && y == player_pos->y - 1)
            || (runch == 'k' && y == player_pos->y + 1)
            || (runch == 'l' && x == player_pos->x - 1)
            || (runch == 'y' && y + x - sumhero >= 1)
            || (runch == 'u' && y - x - diffhero >= 1)
            || (runch == 'n' && y + x - sumhero <= -1)
            || (runch == 'b' && y - x - diffhero <= -1))
          continue;

        switch (xy_ch)
        {
          case DOOR:
            if (x == player_pos->x || y == player_pos->y)
              running = false;
            break;

          case PASSAGE:
            if (x == player_pos->x || y == player_pos->y)
              passcount++;
            break;

          case FLOOR: case VWALL: case HWALL: case SHADOW:
            break;

          default:
            running = false;
            break;
        }
      }
    }
  }

  if (door_stop && !firstmove && passcount > 1)
    running = false;

  if (!running || !jump)
    mvaddcch(player_pos->y, player_pos->x, PLAYER);
}

void
erase_lamp(Coordinate const* pos, struct room const* room)
{
  if (!((room->r_flags & (ISGONE|ISDARK)) == ISDARK
       && !player_is_blind()))
    return;

  Coordinate const* player_pos = player_get_pos();
  for (int x = pos->x -1; x <= pos->x +1; x++)
    for (int y = pos->y -1; y <= pos->y +1; y++)
    {
      if (y == player_pos->y && x == player_pos->x)
        continue;

      move(y, x);
      if (incch() == FLOOR)
        addcch(SHADOW);
    }
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

  if (player_is_confused() && os_rand_range(5) == 0)
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

void
call_it(string const& what, struct obj_info *info)
{
  if (info->oi_know) {
    info->oi_guess.clear();
  } else if (!info->oi_guess.empty()) {
    char tmpbuf[MAXSTR] = { '\0' };
    io_msg("what do you want to name the %s? ", what.c_str());
    if (io_readstr(tmpbuf) == 0) {
      info->oi_guess = tmpbuf;
    }
  }
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
  return (player_get_room()->r_flags & ISGONE)
    ? PASSAGE : FLOOR;
}

char
floor_at(void)
{
  Coordinate *player_pos = player_get_pos();
  char ch = Game::level->get_ch(*player_pos);
  return ch == FLOOR ? floor_ch() : ch;
}

bool
fallpos(Coordinate const* pos, Coordinate* newpos)
{
  int cnt = 0;
  for (int y = pos->y - 1; y <= pos->y + 1; y++)
    for (int x = pos->x - 1; x <= pos->x + 1; x++)
    {
      Coordinate *player_pos = player_get_pos();
      /*
       * check to make certain the spot is empty, if it is,
       * put the object there, set it in the level list
       * and re-draw the room if he can see it
       */
      if (y == player_pos->y && x == player_pos->x)
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


