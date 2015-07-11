/*
 * All sorts of miscellaneous routines
 *
 * @(#)misc.c	4.66 (Berkeley) 08/06/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

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
#include "passages.h"
#include "player.h"
#include "rings.h"
#include "rogue.h"
#include "rooms.h"

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
        if (y != level_stairs.y || x != level_stairs.x || !seen_stairs())
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
  coord const* player_pos = player_get_pos();
  char const player_ch = level_get_place(player_pos->y, player_pos->x)->p_ch;
  char const player_flags = level_get_place(player_pos->y, player_pos->x)->p_flags;

  if (!coord_same(&move_pos_prev, player_pos))
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

      PLACE const* xy_pos = level_get_place(y, x);
      char xy_ch = xy_pos->p_ch;
      if (xy_ch == SHADOW)  /* nothing need be done with a ' ' */
        continue;

      char const xy_flags = xy_pos->p_flags;
      if (player_ch != DOOR
          && xy_ch != DOOR
          && (player_flags & F_PASS) != (xy_flags & F_PASS))
        continue;

      if (((xy_flags & F_PASS) || xy_ch == DOOR)
          && ((player_flags & F_PASS) || player_ch == DOOR))
      {
        if (player_pos->x != x && player_pos->y != y
            && !step_ok(level_get_ch(y, player_pos->x))
            && !step_ok(level_get_ch(player_pos->y, x)))
          continue;
      }

      THING* monster = xy_pos->p_monst;
      if (monster == NULL)
        xy_ch = (char)trip_ch(y, x, xy_ch);
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
              ? (char)(os_rand_range(26) + 'A')
              : monster->t.t_disguise;
          }
        }
      }

      if (player_is_blind() && (y != player_pos->y || x != player_pos->x))
        continue;

      move(y, x);

      if (monster != NULL || xy_ch != incch())
        addcch(xy_ch);

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
erase_lamp(coord const* pos, struct room const* room)
{
  if (!((room->r_flags & (ISGONE|ISDARK)) == ISDARK
       && !player_is_blind()))
    return;

  coord const* player_pos = player_get_pos();
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

THING*
find_obj(int y, int x)
{
  for (THING* obj = level_items; obj != NULL; obj = obj->o.l_next)
    if (obj->o.o_pos.y == y && obj->o.o_pos.x == x)
      return obj;

  /* It should have returned by now */
  io_msg("DEBUG: Non-object %d,%d", y, x);
  return NULL;
}

void
aggravate(void)
{
  for (THING* mp = monster_list; mp != NULL; mp = mp->t.l_next)
    monster_start_running(&mp->t.t_pos);
}

char const*
vowelstr(char const* str)
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

coord const*
get_dir(void)
{
  static coord delta;

  char const* prompt = "which direction? ";
  io_msg(prompt);

  bool gotit;
  do
  {
    gotit = true;
    switch (dir_ch = readchar(false))
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
        return NULL;

      default:
        io_msg_clear();
        io_msg(prompt);
        gotit = false;
        break;
    }
  } while (!gotit);

  if (isupper(dir_ch))
    dir_ch = (char) tolower(dir_ch);

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
call_it(char const* what, struct obj_info *info)
{
  if (info->oi_know)
  {
    if (info->oi_guess != NULL)
    {
      free(info->oi_guess);
      info->oi_guess = NULL;
    }
  }

  else if (!info->oi_guess)
  {
    char tmpbuf[MAXSTR] = { '\0' };
    io_msg("what do you want to name the %s? ", what);
    if (readstr(tmpbuf) == 0)
    {
      if (info->oi_guess != NULL)
        free(info->oi_guess);
      info->oi_guess = malloc((unsigned int) strlen(tmpbuf) + 1);
      strcpy(info->oi_guess, tmpbuf);
    }
  }
}

char
rnd_thing(void)
{
  int i = os_rand_range(level >= level_amulet ? 10 : 9);
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
      if (level < level_amulet)
        (void)fail("rnd_thing: Amulet spawned at a too low level", 0);
      return AMULET;

    default:
      (void)fail("rnd_thing got %d, expected value between 0 and 9", 0);
      return GOLD;
  }
}

bool
is_magic(THING const* obj)
{
  switch (obj->o.o_type)
  {
    case ARMOR:
      return (bool)(obj->o.o_flags & ISPROT) ||
             obj->o.o_arm != armor_ac((enum armor_t)obj->o.o_which);

    case WEAPON: case AMMO:
      return obj->o.o_hplus != 0 || obj->o.o_dplus != 0;

    case POTION: case SCROLL: case STICK: case RING: case AMULET:
      return true;
  }
  return false;
}

bool
seen_stairs(void)
{
  THING* tp = level_get_monster(level_stairs.y, level_stairs.x);

  move(level_stairs.y, level_stairs.x);
  if (incch() == STAIRS)  /* it's on the map */
    return true;
  if (coord_same(player_get_pos(), &level_stairs)) /* It's under him */
    return true;

  /* if a monster is on the stairs, this gets hairy */
  if (tp != NULL)
  {
    /* if it's visible and awake, it must have moved there */
    if (monster_seen_by_player(tp) && monster_is_chasing(tp))
      return true;

    if (player_can_sense_monsters()      /* if she can detect monster */
        && tp->t.t_oldch == STAIRS)        /* and there once were stairs */
      return true;                       /* it must have moved there */
  }
  return false;
}

void
invis_on(void)
{
  player_add_true_sight(true);
  for (THING* mp = monster_list; mp != NULL; mp = mp->t.l_next)
    if (monster_is_invisible(mp) && monster_seen_by_player(mp)
        && !player_is_hallucinating())
      mvaddcch(mp->t.t_pos.y, mp->t.t_pos.x, mp->t.t_disguise);
}


void
strucpy(char* dst, char const* src, int len)
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

void
waste_time(int rounds)
{
  while (rounds--)
  {
    daemon_run_before();
    daemon_run_after();
  }
}

void
set_oldch(THING* tp, coord* cp)
{
  char old_char = tp->t.t_oldch;

  if (coord_same(&tp->t.t_pos, cp))
    return;

  tp->t.t_oldch = mvincch(cp->y, cp->x);
  if (!player_is_blind())
  {
    if ((old_char == FLOOR || tp->t.t_oldch == FLOOR) &&
        (tp->t.t_room->r_flags & ISDARK))
      tp->t.t_oldch = SHADOW;
    else if (dist_cp(cp, player_get_pos()) <= LAMPDIST)
      tp->t.t_oldch = level_get_ch(cp->y, cp->x);
  }
}

struct room*
roomin(coord* cp)
{
  char fp = level_get_flags(cp->y, cp->x);
  if (fp & F_PASS)
    return &passages[fp & F_PNUM];

  for (struct room* rp = rooms; rp < &rooms[ROOMS_MAX]; rp++)
    if (cp->x <= rp->r_pos.x + rp->r_max.x
        && rp->r_pos.x <= cp->x
        && cp->y <= rp->r_pos.y + rp->r_max.y
        && rp->r_pos.y <= cp->y)
      return rp;

  io_msg("in some bizarre place (%d, %d)", cp->y, cp->x);
  assert(0);
  return NULL;
}

bool
diag_ok(coord const* sp, coord const* ep)
{
  if (ep->x < 0 || ep->x >= NUMCOLS || ep->y <= 0 || ep->y >= NUMLINES - 1)
    return false;
  if (ep->x == sp->x || ep->y == sp->y)
    return true;
  return (bool)(step_ok(level_get_ch(ep->y, sp->x))
             && step_ok(level_get_ch(sp->y, ep->x)));
}

bool
cansee(int y, int x)
{
  coord const* player_pos = player_get_pos();

  if (player_is_blind())
    return false;

  if (dist(y, x, player_pos->y, player_pos->x) < LAMPDIST)
  {
    if (level_get_flags(y, x) & F_PASS)
      if (y != player_pos->y && x != player_pos->x &&
          !step_ok(level_get_ch(y, player_pos->x))
          && !step_ok(level_get_ch(player_pos->y, x)))
        return false;
    return true;
  }

  /* We can only see if the hero in the same room as
   * the coordinate and the room is lit or if it is close.  */
  coord tp =
  {
    .y = y,
    .x = x
  };
  struct room const* rer = roomin(&tp);
  if (rer != player_get_room())
    return false;

  return !(rer->r_flags & ISDARK);
}

int
dist(int y1, int x1, int y2, int x2)
{
    return ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

const char *
pick_color(const char *col)
{
  return player_is_hallucinating() ? color_random() : col;
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
  coord *player_pos = player_get_pos();
  char ch = level_get_ch(player_pos->y, player_pos->x);
  return ch == FLOOR ? floor_ch() : ch;
}

bool
fallpos(coord const* pos, coord* newpos)
{
  int cnt = 0;
  for (int y = pos->y - 1; y <= pos->y + 1; y++)
    for (int x = pos->x - 1; x <= pos->x + 1; x++)
    {
      coord *player_pos = player_get_pos();
      /*
       * check to make certain the spot is empty, if it is,
       * put the object there, set it in the level list
       * and re-draw the room if he can see it
       */
      if (y == player_pos->y && x == player_pos->x)
        continue;

      int ch = level_get_ch(y, x);
      if ((ch == FLOOR || ch == PASSAGE) && os_rand_range(++cnt) == 0)
      {
        newpos->y = y;
        newpos->x = x;
      }
    }
  return cnt != 0;
}


