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

#include "status_effects.h"
#include "io.h"
#include "pack.h"
#include "list.h"
#include "monsters.h"
#include "passages.h"
#include "rooms.h"
#include "rogue.h"


/* position matrix for maze positions */
typedef struct spot {
  int nexits;
  coord exits[4];
  int used;
} SPOT;

static int	Maxy, Maxx, Starty, Startx;
static SPOT	maze[NUMLINES/3+1][NUMCOLS/3+1];

#define GOLDGRP 1

/* Draw a vertical line */
static void
room_draw_vertical_line(struct room *rp, int startx)
{
  int y;
  for (y = rp->r_pos.y + 1; y <= rp->r_max.y + rp->r_pos.y - 1; y++)
    chat(y, startx) = VWALL;
}

/* Draw a horizontal line */
static void
room_draw_horizontal_line(struct room *rp, int starty)
{
  int x;
  for (x = rp->r_pos.x; x <= rp->r_pos.x + rp->r_max.x - 1; x++)
    chat(starty, x) = '-';
}

/* Called to illuminate a room.
 * If it is dark, remove anything that might move.  */
static void
room_open_door(struct room *rp)
{
  int y, x;

  if (rp->r_flags & ISGONE)
    return;

  for (y = rp->r_pos.y; y < rp->r_pos.y + rp->r_max.y; y++)
    for (x = rp->r_pos.x; x < rp->r_pos.x + rp->r_max.x; x++)
      if (isupper(winat(y, x)))
        monster_notice_player(y, x);
}

/* Account for maze exits */
static void
room_accnt_maze(int y, int x, int ny, int nx)
{
  SPOT *sp = &maze[y][x];
  coord *cp;

  for (cp = sp->exits; cp < &sp->exits[sp->nexits]; cp++)
    if (cp->y == ny && cp->x == nx)
      return;
  cp->y = ny;
  cp->x = nx;
}

/* Dig out from around where we are now, if possible */
static void
room_dig(int y, int x)
{
  int nexty = 0;
  int nextx = 0;
  static coord pos;
  static coord del[4] = { {2, 0}, {-2, 0}, {0, 2}, {0, -2} };

  for (;;)
  {
    int cnt = 0;
    coord *cp;
    for (cp = del; cp <= &del[3]; cp++)
    {
      int newy = y + cp->y;
      int newx = x + cp->x;
      if (newy < 0 || newy > Maxy || newx < 0 || newx > Maxx)
        continue;
      if (flat(newy + Starty, newx + Startx) & F_PASS)
        continue;
      if (rnd(++cnt) == 0)
      {
        nexty = newy;
        nextx = newx;
      }
    }
    if (cnt == 0)
      return;
    room_accnt_maze(y, x, nexty, nextx);
    room_accnt_maze(nexty, nextx, y, x);
    if (nexty == y)
    {
      pos.y = y + Starty;
      if (nextx - x < 0)
        pos.x = nextx + Startx + 1;
      else
        pos.x = nextx + Startx - 1;
    }
    else
    {
      pos.x = x + Startx;
      if (nexty - y < 0)
        pos.y = nexty + Starty + 1;
      else
        pos.y = nexty + Starty - 1;
    }
    passages_putpass(&pos);
    pos.y = nexty + Starty;
    pos.x = nextx + Startx;
    passages_putpass(&pos);
    room_dig(nexty, nextx);
  }
}

/** rnd_pos:
 * Pick a random spot in a room */
static void
rnd_pos(struct room *rp, coord *cp)
{
  cp->x = rp->r_pos.x + rnd(rp->r_max.x - 2) + 1;
  cp->y = rp->r_pos.y + rnd(rp->r_max.y - 2) + 1;
}

/* Dig a maze */
static void
room_do_maze(struct room *rp)
{
  SPOT *sp;
  int starty, startx;
  static coord pos;

  for (sp = &maze[0][0]; sp <= &maze[NUMLINES / 3][NUMCOLS / 3]; sp++)
  {
    sp->used = false;
    sp->nexits = 0;
  }

  Maxy = rp->r_max.y;
  Maxx = rp->r_max.x;
  Starty = rp->r_pos.y;
  Startx = rp->r_pos.x;
  starty = (rnd(rp->r_max.y) / 2) * 2;
  startx = (rnd(rp->r_max.x) / 2) * 2;
  pos.y = starty + Starty;
  pos.x = startx + Startx;
  passages_putpass(&pos);
  room_dig(starty, startx);
}

/* Draw a box around a room and lay down the floor for normal
 * rooms; for maze rooms, draw maze. */
static void
room_draw(struct room *rp)
{
  if (rp->r_flags & ISMAZE)
    room_do_maze(rp);
  else
  {
    int y;
    int x;
    /* Draw left side */
    room_draw_vertical_line(rp, rp->r_pos.x);
    /* Draw right side */
    room_draw_vertical_line(rp, rp->r_pos.x + rp->r_max.x - 1);
    /* Draw top */
    room_draw_horizontal_line(rp, rp->r_pos.y);
    /* Draw bottom */
    room_draw_horizontal_line(rp, rp->r_pos.y + rp->r_max.y - 1);

    /* Put the floor down */
    for (y = rp->r_pos.y + 1; y < rp->r_pos.y + rp->r_max.y - 1; y++)
      for (x = rp->r_pos.x + 1; x < rp->r_pos.x + rp->r_max.x - 1; x++)
        chat(y, x) = FLOOR;
  }
}

void
rooms_create(void)
{
  int i;
  struct room *rp;
  int left_out;
  coord bsze;				/* maximum room size */

  bsze.x = NUMCOLS / 3;
  bsze.y = NUMLINES / 3;

  /* Clear things for a new level */
  for (rp = rooms; rp < &rooms[MAXROOMS]; rp++)
  {
    rp->r_goldval = 0;
    rp->r_nexits = 0;
    rp->r_flags = 0;
  }

  /* Put the gone rooms, if any, on the level */
  left_out = rnd(4);
  for (i = 0; i < left_out; i++)
    rooms[room_random()].r_flags |= ISGONE;

  /* dig and populate all the rooms on the level */
  for (i = 0, rp = rooms; i < MAXROOMS; rp++, i++)
  {
    /* Find upper left corner of box that this room goes in */
    static coord top;
    top.x = (i % 3) * bsze.x + 1;
    top.y = (i / 3) * bsze.y;

    if (rp->r_flags & ISGONE)
    {
      /** Place a gone room.  Make certain that there is a blank line
       * for passage drawing.  */
      do
      {
        rp->r_pos.x = top.x + rnd(bsze.x - 3) + 1;
        rp->r_pos.y = top.y + rnd(bsze.y - 2) + 1;
        rp->r_max.x = -NUMCOLS;
        rp->r_max.y = -NUMLINES;
      } while (!(rp->r_pos.y > 0 && rp->r_pos.y < NUMLINES-1));
      continue;
    }

    /* set room type */
    if (rnd(10) < level - 1)
    {
      rp->r_flags |= ISDARK;		/* dark room */
      if (rnd(15) == 0)
        rp->r_flags = ISMAZE;		/* maze room */
    }

    /* Find a place and size for a random room */
    if (rp->r_flags & ISMAZE)
    {
      rp->r_max.x = bsze.x - 1;
      rp->r_max.y = bsze.y - 1;
      if ((rp->r_pos.x = top.x) == 1)
        rp->r_pos.x = 0;
      if ((rp->r_pos.y = top.y) == 0)
      {
        rp->r_pos.y++;
        rp->r_max.y--;
      }
    }
    else
      do
      {
        rp->r_max.x = rnd(bsze.x - 4) + 4;
        rp->r_max.y = rnd(bsze.y - 4) + 4;
        rp->r_pos.x = top.x + rnd(bsze.x - rp->r_max.x);
        rp->r_pos.y = top.y + rnd(bsze.y - rp->r_max.y);
      } while (rp->r_pos.y == 0);
    room_draw(rp);

    /* Put the gold in */
    if (rnd(2) == 0 && (!pack_contains_amulet() || level >= max_level))
    {
      THING *gold;

      gold = new_item();
      gold->o_goldval = rp->r_goldval = GOLDCALC;
      room_find_floor(rp, &rp->r_gold, false, false);
      gold->o_pos = rp->r_gold;
      chat(rp->r_gold.y, rp->r_gold.x) = GOLD;
      gold->o_flags = ISMANY;
      gold->o_group = GOLDGRP;
      gold->o_type = GOLD;
      attach(lvl_obj, gold);
    }

    /* Put the monster in */
    if (rnd(100) < (rp->r_goldval > 0 ? 80 : 25))
    {
      coord mp;
      THING *tp = new_item();
      room_find_floor(rp, &mp, false, true);
      monster_new(tp, monster_random(false), &mp);
      monster_give_pack(tp);
    }
  }
}

bool
room_find_floor(struct room *rp, coord *cp, int limit, bool monst)
{
  int cnt = limit;
  char compchar = 0;
  bool pickroom = rp == NULL;

  if (!pickroom)
    compchar = ((rp->r_flags & ISMAZE) ? PASSAGE : FLOOR);

  for (;;)
  {
    PLACE *pp;
    if (limit && cnt-- == 0)
      return false;

    if (pickroom)
    {
      rp = &rooms[room_random()];
      compchar = ((rp->r_flags & ISMAZE) ? PASSAGE : FLOOR);
    }

    rnd_pos(rp, cp);
    pp = INDEX(cp->y, cp->x);
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
room_enter(coord *cp)
{
  struct room *rp = proom = roomin(cp);

  room_open_door(rp);
  if (!(rp->r_flags & ISDARK) && !is_blind(&player))
  {
    int y, x;
    for (y = rp->r_pos.y; y < rp->r_max.y + rp->r_pos.y; y++)
    {
      move(y, rp->r_pos.x);
      for (x = rp->r_pos.x; x < rp->r_max.x + rp->r_pos.x; x++)
      {
        THING *tp = moat(y, x);
        char ch = chat(y, x);
        if (tp == NULL)
          if (ch != incch())
            addcch(ch);
          else
            move(y, x + 1);
        else
        {
          tp->t_oldch = ch;
          if (!see_monst(tp))
            if (on(player, SEEMONST))
              addcch(tp->t_disguise | A_STANDOUT);
            else
              addcch(ch);
          else
            addcch(tp->t_disguise);
        }
      }
    }
  }
}

/** room_leave:
 * Code for when we exit a room */
void
room_leave(coord *cp)
{
    struct room *rp = proom;
    int y, x;
    char floor;

    if (rp->r_flags & ISMAZE)
	return;

    if (rp->r_flags & ISGONE)
	floor = PASSAGE;
    else if (!(rp->r_flags & ISDARK) || is_blind(&player))
	floor = FLOOR;
    else
	floor = SHADOW;

    proom = &passages[flat(cp->y, cp->x) & F_PNUM];
    for (y = rp->r_pos.y; y < rp->r_max.y + rp->r_pos.y; y++)
	for (x = rp->r_pos.x; x < rp->r_max.x + rp->r_pos.x; x++)
	{
	    char ch;
	    move(y, x);
	    switch (ch = incch())
	    {
		case FLOOR:
		    if (floor == SHADOW && ch != SHADOW)
			addcch(SHADOW);
		    break;
		default:
		    /*
		     * to check for monster, we have to strip out
		     * standout bit
		     */
		    if (isupper(toascii(ch)))
		    {
                      PLACE *pp = INDEX(y,x);
			if (on(player, SEEMONST))
			{
			    addcch(ch | A_STANDOUT);
			    break;
			}
			addcch(pp->p_ch == DOOR ? DOOR : floor);
		    }
	    }
	}
    room_open_door(rp);
}

int
room_random(void)
{
  int rm;

  do
    rm = rnd(MAXROOMS);
  while (rooms[rm].r_flags & ISGONE);

  return rm;
}
