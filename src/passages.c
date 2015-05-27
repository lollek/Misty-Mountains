/*
 * Draw the connecting passages
 *
 * @(#)passages.c	4.22 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>

#include "io.h"
#include "misc.h"
#include "rooms.h"
#include "level.h"
#include "rogue.h"

#include "passages.h"

/* One for each passage */
struct room passages[] = {
/*    r_pos   r_max   r_gold r_goldval r_flags        r_nexits r_exit[12] */
    { {0, 0}, {0, 0}, {0, 0}, 0,       ISGONE|ISDARK, 0,       {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0,       ISGONE|ISDARK, 0,       {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0,       ISGONE|ISDARK, 0,       {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0,       ISGONE|ISDARK, 0,       {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0,       ISGONE|ISDARK, 0,       {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0,       ISGONE|ISDARK, 0,       {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0,       ISGONE|ISDARK, 0,       {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0,       ISGONE|ISDARK, 0,       {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0,       ISGONE|ISDARK, 0,       {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0,       ISGONE|ISDARK, 0,       {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0,       ISGONE|ISDARK, 0,       {{0,0}} },
    { {0, 0}, {0, 0}, {0, 0}, 0,       ISGONE|ISDARK, 0,       {{0,0}} }
};

static int pnum;
static bool newpnum;

/** numpass:
 * Number a passageway square and its brethren */
static void
numpass(int y, int x)
{
  if (x >= NUMCOLS || x < 0 || y >= NUMLINES || y <= 0)
    return;

  int flags = level_get_flags(y, x);
  if (flags & F_PNUM)
    return;

  if (newpnum)
  {
    pnum++;
    newpnum = false;
  }

  /* check to see if it is a door or secret door, i.e., a new exit,
   * or a numerable type of place */
  char ch = level_get_ch(y, x);
  if (ch == DOOR || (!(flags & F_REAL) && (ch == VWALL || ch == HWALL)))
  {
    struct room* rp = &passages[pnum];
    rp->r_exit[rp->r_nexits].y = y;
    rp->r_exit[rp->r_nexits++].x = x;
  }

  else if (!(flags & F_PASS))
    return;

  flags |= pnum;
  level_set_flags(y, x, (char)flags);

  /* recurse on the surrounding places */
  numpass(y + 1, x);
  numpass(y - 1, x);
  numpass(y, x + 1);
  numpass(y, x - 1);
}

/** passnum:
 * Assign a number to each passageway */
static void
passnum(void)
{
  pnum = 0;
  newpnum = false;
  for (struct room* rp = passages; rp < &passages[MAXPASS]; rp++)
    rp->r_nexits = 0;
  for (struct room* rp = rooms; rp < &rooms[MAXROOMS]; rp++)
    for (int i = 0; i < rp->r_nexits; i++)
    {
      newpnum++;
      numpass(rp->r_exit[i].y, rp->r_exit[i].x);
    }
}


/** door:
 * Add a door or possibly a secret door.  Also enters the door in
 * the exits array of the room.  */
static void
door(struct room* rm, coord* cp)
{
  rm->r_exit[rm->r_nexits++] = *cp;
  if (rm->r_flags & ISMAZE)
    return;

  PLACE* place = INDEX(cp->y, cp->x);
  if (rnd(10) + 1 < level && rnd(5) == 0)
  {
    if (cp->y == rm->r_pos.y || cp->y == rm->r_pos.y + rm->r_max.y - 1)
      place->p_ch = HWALL;
    else
      place->p_ch = VWALL;
    place->p_flags &= ~F_REAL;
  }
  else
    place->p_ch = DOOR;
}


/** conn:
 * Draw a corridor from a room in a certain direction. */
static void
conn(int r1, int r2)
{
  int rm;
  char direc;
  if (r1 < r2)
  {
    rm = r1;
    if (r1 + 1 == r2)
      direc = 'r';
    else
      direc = 'd';
  }
  else
  {
    rm = r2;
    if (r2 + 1 == r1)
      direc = 'r';
    else
      direc = 'd';
  }

  struct room* room_from = &rooms[rm];
  struct room* room_to = NULL;           /* room pointer of dest */
  coord start_pos;                       /* start of move */
  coord end_pos;                         /* end of move */
  coord turn_delta;                      /* direction to turn */
  coord del;                             /* direction of move */
  int distance = 0;                      /* distance to move */
  int turn_distance = 0;                 /* how far to turn */

    /* Set up the movement variables, in two cases:
     * first drawing one down.  */
  if (direc == 'd')
  {
    room_to = &rooms[rm + 3];
    del.x = 0;
    del.y = 1;
    start_pos.x = room_from->r_pos.x;
    start_pos.y = room_from->r_pos.y;
    end_pos.x = room_to->r_pos.x;
    end_pos.y = room_to->r_pos.y;

    /* if not gone pick door pos */
    if (!(room_from->r_flags & ISGONE))
      do
      {
        start_pos.x = room_from->r_pos.x + rnd(room_from->r_max.x - 2) + 1;
        start_pos.y = room_from->r_pos.y + room_from->r_max.y - 1;
      }
      while ((room_from->r_flags & ISMAZE)
             && !(level_get_flags(start_pos.y, start_pos.x) & F_PASS));

    if (!(room_to->r_flags & ISGONE))
      do
        end_pos.x = room_to->r_pos.x + rnd(room_to->r_max.x - 2) + 1;
      while ((room_to->r_flags & ISMAZE)
             && !(level_get_flags(end_pos.y, end_pos.x) & F_PASS));

    distance = abs(start_pos.y - end_pos.y) - 1;
    turn_delta.y = 0;
    turn_delta.x = (start_pos.x < end_pos.x ? 1 : -1);

    turn_distance = abs(start_pos.x - end_pos.x);
  }

  /* setup for moving right */
  else if (direc == 'r')
  {
    room_to = &rooms[rm + 1];
    del.x = 1;
    del.y = 0;
    start_pos.x = room_from->r_pos.x;
    start_pos.y = room_from->r_pos.y;
    end_pos.x = room_to->r_pos.x;
    end_pos.y = room_to->r_pos.y;
    if (!(room_from->r_flags & ISGONE))
      do
      {
        start_pos.x = room_from->r_pos.x + room_from->r_max.x - 1;
        start_pos.y = room_from->r_pos.y + rnd(room_from->r_max.y - 2) + 1;
      } while ((room_from->r_flags & ISMAZE)
               && !(level_get_flags(start_pos.y, start_pos.x) & F_PASS));

    if (!(room_to->r_flags & ISGONE))
      do
        end_pos.y = room_to->r_pos.y + rnd(room_to->r_max.y - 2) + 1;
      while ((room_to->r_flags & ISMAZE)
             && !(level_get_flags(end_pos.y, end_pos.x) & F_PASS));

    distance = abs(start_pos.x - end_pos.x) - 1;
    turn_delta.y = (start_pos.y < end_pos.y ? 1 : -1);
    turn_delta.x = 0;
    turn_distance = abs(start_pos.y - end_pos.y);
  }

  else
    msg("DEBUG: error in connection tables");

  /* where turn starts */
  int turn_spot = rnd(distance - 1) + 1;

  /* Draw in the doors on either side of the passage or just put #'s
   * if the rooms are gone.  */
  if (!(room_from->r_flags & ISGONE))
    door(room_from, &start_pos);
  else
    passages_putpass(&start_pos);

  if (!(room_to->r_flags & ISGONE))
    door(room_to, &end_pos);
  else
    passages_putpass(&end_pos);

  /* Get ready to move...  */
  coord curr =
  {
    .x = start_pos.x,
    .y = start_pos.y
  };
  while (distance > 0)
  {
    /* Move to new position */
    curr.x += del.x;
    curr.y += del.y;

    /* Check if we are at the turn place, if so do the turn */
    if (distance == turn_spot)
      while (turn_distance--)
      {
        passages_putpass(&curr);
        curr.x += turn_delta.x;
        curr.y += turn_delta.y;
      }

    /* Continue digging along */
    passages_putpass(&curr);
    distance--;
  }

  curr.x += del.x;
  curr.y += del.y;

  if (!same_coords(&curr, &end_pos))
    msg("warning, connectivity problem on this level");
}


/** passages_do:
 * Draw all the passages on a level.  */
void
passages_do(void)
{
  struct rdes
  {
    bool conn[MAXROOMS];  /* possible to connect to room i? */
    bool isconn[MAXROOMS];/* connection been made to room i? */
    bool ingraph;         /* this room in graph already? */
  } rdes[MAXROOMS] = {
    { { 0, 1, 0, 1, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 1, 0, 1, 0, 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 0, 1, 0, 0, 0, 1, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 1, 0, 0, 0, 1, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 0, 1, 0, 1, 0, 1, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 0, 0, 1, 0, 1, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 0, 0, 0, 1, 0, 0, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 0, 0, 0, 0, 1, 0, 1, 0, 1 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 0, 0, 0, 0, 0, 1, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
  };

  /* reinitialize room graph description */
  for (struct rdes* ptr = rdes; ptr <= &rdes[MAXROOMS-1]; ptr++)
  {
    for (int i = 0; i < MAXROOMS; i++)
      ptr->isconn[i] = false;
    ptr->ingraph = false;
  }

  /* starting with one room, connect it to a random adjacent room and
   * then pick a new room to start with.  */
  int roomcount = 1;
  struct rdes* r1 = &rdes[rnd(MAXROOMS)];
  r1->ingraph = true;

  struct rdes* r2 = NULL;
  do
    {
      /* find a room to connect with */
      int j = 0;
      for (int i = 0; i < MAXROOMS; i++)
        if (r1->conn[i] && !rdes[i].ingraph && !rnd(++j))
          r2 = &rdes[i];

      /* if no adjacent rooms are outside the graph, pick a new room
       * to look from */
      if (j == 0)
      {
        do
          r1 = &rdes[rnd(MAXROOMS)];
        while (!r1->ingraph);
      }

      /* otherwise, connect new room to the graph, and draw a tunnel
       * to it */
      else
      {
        r2->ingraph = true;
        int i = (int)(r1 - rdes);
        j = (int)(r2 - rdes);
        conn(i, j);
        r1->isconn[j] = true;
        r2->isconn[i] = true;
        roomcount++;
      }
    } while (roomcount < MAXROOMS);

    /* attempt to add passages to the graph a random number of times so
     * that there isn't always just one unique passage through it.  */
  for (roomcount = rnd(5); roomcount > 0; roomcount--)
  {
    r1 = &rdes[rnd(MAXROOMS)];	/* a random room to look from */

    /* find an adjacent room not already connected */
    int j = 0;
    for (int i = 0; i < MAXROOMS; i++)
      if (r1->conn[i] && !r1->isconn[i] && rnd(++j) == 0)
        r2 = &rdes[i];

    /* if there is one, connect it and look for the next added passage */
    if (j != 0)
    {
      int i = (int)(r1 - rdes);
      j = (int)(r2 - rdes);
      conn(i, j);
      r1->isconn[j] = true;
      r2->isconn[i] = true;
    }
  }
  passnum();
}

/** passages_putpass:
 * add a passage character or secret passage here */
void
passages_putpass(coord* cp)
{
  PLACE *pp = INDEX(cp->y, cp->x);
  pp->p_flags |= F_PASS;

  if (rnd(10) + 1 < level && rnd(40) == 0)
    pp->p_flags &= ~F_REAL;
  else
    pp->p_ch = PASSAGE;
}

/** passages_add_pass:
 * Add the passages to the current window (wizard command) */
void
passages_add_pass(void)
{
  for (int y = 1; y < NUMLINES - 1; y++)
    for (int x = 0; x < NUMCOLS; x++)
    {
      PLACE* pp = INDEX(y, x);
      if ((pp->p_flags & F_PASS) || pp->p_ch == DOOR ||
          (!(pp->p_flags&F_REAL) && (pp->p_ch == VWALL ||
                                     pp->p_ch == HWALL)))
      {
        char ch = pp->p_ch;
        if (pp->p_flags & F_PASS)
          ch = PASSAGE;
        pp->p_flags |= F_SEEN;
        move(y, x);
        if (pp->p_monst != NULL)
          pp->p_monst->t_oldch = pp->p_ch;
        else if (pp->p_flags & F_REAL)
          addcch(ch);
        else
        {
          standout();
          addcch((pp->p_flags & F_PASS) ? PASSAGE : DOOR);
          standend();
        }
      }
    }
}
