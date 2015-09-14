/*
 * Create the layout for the new level
 *
 * @(#)rooms.c	4.45 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */
#include <ctype.h>

#include "io.h"
#include "pack.h"
#include "list.h"
#include "monster.h"
#include "passages.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "os.h"
#include "rogue.h"

#include "rooms.h"

struct room* room_prev;
struct room rooms[ROOMS_MAX];

/* position matrix for maze positions */
typedef struct spot {
  int nexits;
  coord exits[4];
  int used;
} SPOT;

static SPOT maze[NUMLINES/3+1][NUMCOLS/3+1];

/* Draw a vertical line */
static void
room_draw_vertical_line(struct room* rp, int startx)
{
  for (int y = rp->r_pos.y + 1; y <= rp->r_max.y + rp->r_pos.y - 1; y++)
    level_set_ch(y, startx, VWALL);
}

/* Draw a horizontal line */
static void
room_draw_horizontal_line(struct room* rp, int starty)
{
  for (int x = rp->r_pos.x; x <= rp->r_pos.x + rp->r_max.x - 1; x++)
    level_set_ch(starty, x, '-');
}

/* Called to illuminate a room.
 * If it is dark, remove anything that might move.  */
static void
room_open_door(struct room* rp)
{
  if (rp->r_flags & ISGONE)
    return;

  for (int y = rp->r_pos.y; y < rp->r_pos.y + rp->r_max.y; y++)
    for (int x = rp->r_pos.x; x < rp->r_pos.x + rp->r_max.x; x++)
      if (isupper(level_get_type(y, x)))
        monster_notice_player(y, x);
}

/* Account for maze exits */
static void
room_accnt_maze(int y, int x, int ny, int nx)
{
  SPOT* sp = &maze[y][x];
  coord* cp;

  for (cp = sp->exits; cp < &sp->exits[sp->nexits]; cp++)
    if (cp->y == ny && cp->x == nx)
      return;

  cp->y = ny;
  cp->x = nx;
}


/* Dig out from around where we are now, if possible */
static void
room_dig(int y, int x, int starty, int startx, int maxy, int maxx)
{
  int nexty = 0;
  int nextx = 0;

  for (;;)
  {
    coord const del[4] = { {2, 0}, {-2, 0}, {0, 2}, {0, -2} };
    int cnt = 0;
    for (unsigned i = 0; i < sizeof(del)/sizeof(*del); ++i)
    {
      int newy = y + del[i].y;
      int newx = x + del[i].x;

      if (newy < 0 || newy > maxy || newx < 0 || newx > maxx
          || level_get_flags(newy + starty, newx + startx) & F_PASS)
        continue;

      if (os_rand_range(++cnt) == 0)
      {
        nexty = newy;
        nextx = newx;
      }
    }

    if (cnt == 0)
      return;

    room_accnt_maze(y, x, nexty, nextx);
    room_accnt_maze(nexty, nextx, y, x);

    coord pos;
    if (nexty == y)
    {
      pos.y = y + starty;
      if (nextx - x < 0)
        pos.x = nextx + startx + 1;
      else
        pos.x = nextx + startx - 1;
    }
    else
    {
      pos.x = x + startx;
      if (nexty - y < 0)
        pos.y = nexty + starty + 1;
      else
        pos.y = nexty + starty - 1;
    }
    passages_putpass(&pos);

    pos.y = nexty + starty;
    pos.x = nextx + startx;
    passages_putpass(&pos);

    room_dig(nexty, nextx, starty, startx, maxy, maxx);
  }
}

/** rnd_pos:
 * Pick a random spot in a room */
static void
rnd_pos(struct room* rp, coord* cp)
{
  cp->x = rp->r_pos.x + os_rand_range(rp->r_max.x - 2) + 1;
  cp->y = rp->r_pos.y + os_rand_range(rp->r_max.y - 2) + 1;
}

/* Dig a maze */
static void
room_do_maze(struct room* rp)
{
  for (SPOT* sp = &maze[0][0]; sp <= &maze[NUMLINES / 3][NUMCOLS / 3]; sp++)
  {
    sp->used = false;
    sp->nexits = 0;
  }

  int y = (os_rand_range(rp->r_max.y) / 2) * 2;
  int x = (os_rand_range(rp->r_max.x) / 2) * 2;

  coord pos = {
    .y = y + rp->r_pos.y,
    .x = y + rp->r_pos.x
  };
  passages_putpass(&pos);

  room_dig(y, x, rp->r_pos.y, rp->r_pos.x, rp->r_max.y, rp->r_max.x);
}

/* Draw a box around a room and lay down the floor for normal
 * rooms; for maze rooms, draw maze. */
static void
room_draw(struct room* rp)
{
  if (rp->r_flags & ISMAZE)
  {
    room_do_maze(rp);
    return;
  }

  /* Draw left side */
  room_draw_vertical_line(rp, rp->r_pos.x);
  /* Draw right side */
  room_draw_vertical_line(rp, rp->r_pos.x + rp->r_max.x - 1);
  /* Draw top */
  room_draw_horizontal_line(rp, rp->r_pos.y);
  /* Draw bottom */
  room_draw_horizontal_line(rp, rp->r_pos.y + rp->r_max.y - 1);

  /* Put the floor down */
  for (int y = rp->r_pos.y + 1; y < rp->r_pos.y + rp->r_max.y - 1; y++)
    for (int x = rp->r_pos.x + 1; x < rp->r_pos.x + rp->r_max.x - 1; x++)
      level_set_ch(y, x, FLOOR);
}

static void
room_place_gone_room(coord const* max_size, coord const* top, struct room* room)
{
  /** Place a gone room.  Make certain that there is a blank line
   * for passage drawing.  */
  do
  {
    room->r_pos.x = top->x + os_rand_range(max_size->x - 3) + 1;
    room->r_pos.y = top->y + os_rand_range(max_size->y - 2) + 1;
    room->r_max.x = -NUMCOLS;
    room->r_max.y = -NUMLINES;
  }
  while (!(room->r_pos.y > 0 && rooms->r_pos.y < NUMLINES-1));
}

void
rooms_create(void)
{
  /* maximum room size */
  coord const bsze = {
    .x = NUMCOLS / 3,
    .y = NUMLINES / 3
  };

  /* Clear things for a new level */
  for (int i = 0; i < ROOMS_MAX; ++i)
  {
    rooms[i].r_goldval = 0;
    rooms[i].r_nexits = 0;
    rooms[i].r_flags = 0;
  }

  /* Put the gone rooms, if any, on the level */
  int left_out = os_rand_range(4);
  for (int i = 0; i < left_out; i++)
    rooms[room_random()].r_flags |= ISGONE;

  /* dig and populate all the rooms on the level */
  for (int i = 0; i < ROOMS_MAX; i++)
  {
    /* Find upper left corner of box that this room goes in */
    coord const top = {
      .x = (i % 3) * bsze.x + 1,
      .y = (i / 3) * bsze.y
    };

    if (rooms[i].r_flags & ISGONE)
    {
      room_place_gone_room(&bsze, &top, &rooms[i]);
      continue;
    }

    /* set room type */
    if (os_rand_range(10) < level - 1)
    {
      rooms[i].r_flags |= ISDARK;  /* dark room */
      if (os_rand_range(15) == 0)
        rooms[i].r_flags = ISMAZE; /* maze room */
    }

    /* Find a place and size for a random room */
    if (rooms[i].r_flags & ISMAZE)
    {
      rooms[i].r_max.x = bsze.x - 1;
      rooms[i].r_max.y = bsze.y - 1;
      rooms[i].r_pos.x = top.x == 1 ? 0 : top.x ;
      rooms[i].r_pos.y = top.y;
      if (rooms[i].r_pos.y == 0)
      {
        rooms[i].r_pos.y++;
        rooms[i].r_max.y--;
      }
    }

    else
      do
      {
        rooms[i].r_max.x = os_rand_range(bsze.x - 4) + 4;
        rooms[i].r_max.y = os_rand_range(bsze.y - 4) + 4;
        rooms[i].r_pos.x = top.x + os_rand_range(bsze.x - rooms[i].r_max.x);
        rooms[i].r_pos.y = top.y + os_rand_range(bsze.y - rooms[i].r_max.y);
      } while (rooms[i].r_pos.y == 0);
    room_draw(&rooms[i]);

    /* Put the gold in */
    if (os_rand_range(2) == 0 && (!pack_contains_amulet() || level >= level_max))
    {
      THING *gold = os_calloc_thing();
      gold->o.o_goldval = rooms[i].r_goldval = GOLDCALC;
      room_find_floor(&rooms[i], &rooms[i].r_gold, false, false);
      gold->o.o_pos = rooms[i].r_gold;
      level_set_ch(rooms[i].r_gold.y, rooms[i].r_gold.x, GOLD);
      gold->o.o_flags = ISMANY;
      gold->o.o_type = GOLD;
      list_attach(&level_items, gold);
    }

    /* Put the monster in */
    if (os_rand_range(100) < (rooms[i].r_goldval > 0 ? 80 : 25))
    {
      coord mp;
      room_find_floor(&rooms[i], &mp, false, true);
      THING *tp = os_calloc_thing();
      monster_new(tp, monster_random(false), &mp);
      monster_give_pack(&tp->t);
    }
  }
}

bool
room_find_floor(struct room* rp, coord* cp, int limit, bool monst)
{
  int cnt = limit;
  char compchar = 0;
  bool pickroom = rp == NULL;

  if (!pickroom)
    compchar = ((rp->r_flags & ISMAZE) ? PASSAGE : FLOOR);

  for (;;)
  {
    if (limit && cnt-- == 0)
      return false;

    if (pickroom)
    {
      rp = &rooms[room_random()];
      compchar = ((rp->r_flags & ISMAZE) ? PASSAGE : FLOOR);
    }

    rnd_pos(rp, cp);
    PLACE* pp = level_get_place(cp->y, cp->x);
    if (monst)
    {
      if (pp->p_monst == NULL && step_ok(pp->p_ch))
        return true;
    }
    else if (pp->p_ch == compchar)
      return true;
  }
}

void
room_enter(coord* cp)
{
  struct room* rp = roomin(cp);
  player_set_room(rp);
  room_open_door(rp);

  if (!(rp->r_flags & ISDARK) && !player_is_blind())
    for (int y = rp->r_pos.y; y < rp->r_max.y + rp->r_pos.y; y++)
    {
      move(y, rp->r_pos.x);
      for (int x = rp->r_pos.x; x < rp->r_max.x + rp->r_pos.x; x++)
      {
        THING *tp = level_get_monster(y, x);
        char ch = level_get_ch(y, x);

        if (tp == NULL)
        {
          mvaddcch(y, x, (chtype) ch);
          continue;
        }

        tp->t.t_oldch = ch;
        if (monster_seen_by_player(&tp->t))
          mvaddcch(y, x, (chtype) tp->t.t_disguise);
        else if (player_can_sense_monsters())
          mvaddcch(y, x, (chtype) tp->t.t_disguise | A_STANDOUT);
        else
          mvaddcch(y, x, (chtype) ch);
      }
    }
}

/** room_leave:
 * Code for when we exit a room */
void
room_leave(coord* cp)
{
  struct room* rp = player_get_room();

  if (rp->r_flags & ISMAZE)
    return;

  char floor;
  if (rp->r_flags & ISGONE)
    floor = PASSAGE;
  else if (!(rp->r_flags & ISDARK) || player_is_blind())
    floor = FLOOR;
  else
    floor = SHADOW;

  player_set_room(&passages[level_get_flags(cp->y, cp->x) & F_PNUM]);
  for (int y = rp->r_pos.y; y < rp->r_max.y + rp->r_pos.y; y++)
    for (int x = rp->r_pos.x; x < rp->r_max.x + rp->r_pos.x; x++)
    {
      move(y, x);
      char ch = (char) incch();

      if (ch == FLOOR)
      {
        if (floor == SHADOW)
          mvaddcch(y, x, SHADOW);
      }

      /* Don't touch non-monsters */
      else if (!isupper(ch))
        continue;

      if (player_can_sense_monsters())
        mvaddcch(y, x, (chtype) ch | A_STANDOUT);
      else
        mvaddcch(y, x, (chtype) (level_get_ch(y, x) == DOOR ? DOOR : floor));
    }

  room_open_door(rp);
}

int
room_random(void)
{
  int rm;

  do
    rm = os_rand_range(ROOMS_MAX);
  while (rooms[rm].r_flags & ISGONE);

  return rm;
}
