/*
 * level:
 *	Dig and draw a new level
 *
 * @(#)level.c	4.38 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <string.h>
#include <stdbool.h>

#include <list>

using namespace std;

#include "traps.h"
#include "io.h"
#include "pack.h"
#include "daemons.h"
#include "monster.h"
#include "passages.h"
#include "misc.h"
#include "player.h"
#include "rooms.h"
#include "things.h"
#include "os.h"
#include "rogue.h"

#include "level.h"

/* Tuneables */
#define MAXOBJ		9  /* How many attempts to put items in dungeon */
#define TREAS_ROOM	20 /* one chance in TREAS_ROOM for a treasure room */
#define MAXTREAS	10 /* maximum number of treasures in a treasure room */
#define MINTREAS	2  /* minimum number of treasures in a treasure room */
#define MAXTRIES	10 /* max number of tries to put down a monster */
#define MAXTRAPS	10

int const level_amulet = 26;

PLACE          level_places[MAXLINES*MAXCOLS];
Coordinate     level_stairs;
list<item*>    level_items;
int            level = 1;
int            level_max = 1;
int            levels_without_food = 0;


/** treas_room:
 * Add a treasure room */
static void
treas_room(void)
{
  struct room* room = &rooms[room_random()];
  int spots = (room->r_max.y - 2) * (room->r_max.x - 2) - MINTREAS;

  if (spots > (MAXTREAS - MINTREAS))
    spots = (MAXTREAS - MINTREAS);
  int num_monsters = os_rand_range(spots) + MINTREAS;

  for (int i = 0; i < num_monsters; ++i)
  {
    Coordinate monster_pos;
    item* item = new_thing();

    room_find_floor(room, &monster_pos, 2 * MAXTRIES, false);
    item->o_pos = monster_pos;
    level_items.push_back(item);
    level_set_ch(monster_pos.y, monster_pos.x, static_cast<char>(item->o_type));
  }

  /* fill up room with monsters from the next level down */
  int nm = os_rand_range(spots) + MINTREAS;
  if (nm < num_monsters + 2)
    nm = num_monsters + 2;

  spots = (room->r_max.y - 2) * (room->r_max.x - 2);
  if (nm > spots)
    nm = spots;
  level++;
  while (nm--)
  {
    Coordinate monster_pos;
    spots = 0;
    if (room_find_floor(room, &monster_pos, MAXTRIES, true))
    {
      monster* tp = new monster();
      monster_new(tp, monster_random(false), &monster_pos);
      tp->t_flags |= ISMEAN;	/* no sloughers in THIS room */
      monster_give_pack(tp);
    }
  }
  level--;
}

/** put_things:
 * Put potions and scrolls on this level */
static void
put_things(void)
{
  int i;

  /* Once you have found the amulet, the only way to get new stuff is
   * go down into the dungeon. */
  if (pack_contains_amulet() && level < level_max)
      return;

  /* check for treasure rooms, and if so, put it in. */
  if (os_rand_range(TREAS_ROOM) == 0)
    treas_room();

  /* Do MAXOBJ attempts to put things on a level */
  for (i = 0; i < MAXOBJ; i++)
    if (os_rand_range(100) < 36)
    {
      /* Pick a new object and link it in the list */
      item* obj = new_thing();
      level_items.push_back(obj);

      /* Put it somewhere */
      room_find_floor(nullptr, &obj->o_pos, false, false);
      level_set_ch(obj->o_pos.y, obj->o_pos.x, static_cast<char>(obj->o_type));
    }

  /* If he is really deep in the dungeon and he hasn't found the
   * amulet yet, put it somewhere on the ground */
  if (level >= level_amulet && !pack_contains_amulet())
  {
    item* amulet = new_amulet();
    level_items.push_back(amulet);

    /* Put it somewhere */
    room_find_floor(nullptr, &amulet->o_pos, false, false);
    level_set_ch(amulet->o_pos.y, amulet->o_pos.x, AMULET);
  }
}


void
level_new(void)
{
  /* unhold when you go down just in case */
  player_remove_held();

  /* Set max level we've been to */
  if (level > level_max)
    level_max = level;

  /* Clean things off from last level */
  for (PLACE* pp = level_places; pp < &level_places[MAXCOLS*MAXLINES]; pp++)
  {
      pp->p_ch = SHADOW;
      pp->p_flags = F_REAL;
      pp->p_monst = nullptr;
  }
  clear();

  /* Free up the monsters on the last level */
  monster_remove_all();

  /* Throw away stuff left on the previous level (if anything) */
  level_items.clear();

  rooms_create(); /* Draw rooms */
  passages_do();  /* Draw passages */
  levels_without_food++;      /* Levels with no food placed */
  put_things();   /* Place objects (if any) */

  /* Place the traps */
  if (os_rand_range(10) < level)
  {
    int ntraps = os_rand_range(level / 4) + 1;
    if (ntraps > MAXTRAPS)
      ntraps = MAXTRAPS;
    while (ntraps--)
    {
      /*
       * not only wouldn't it be NICE to have traps in mazes
       * (not that we care about being nice), since the trap
       * number is stored where the passage number is, we
       * can't actually do it.
       */
      do
        room_find_floor(nullptr, &level_stairs, false, false);
      while (level_get_ch(level_stairs.y, level_stairs.x) != FLOOR);

      char trapflag = level_get_flags(level_stairs.y, level_stairs.x);
      trapflag &= ~F_REAL;
      trapflag |= os_rand_range(NTRAPS);
      level_set_flags(level_stairs.y, level_stairs.x, trapflag);
    }
  }

  /* Place the staircase down.  */
  room_find_floor(nullptr, &level_stairs, false, false);
  level_set_ch(level_stairs.y, level_stairs.x, STAIRS);

  monster_set_all_rooms();

  Coordinate* player_pos = player_get_pos();
  room_find_floor(nullptr, player_pos, false, true);
  room_enter(player_pos);
  mvaddcch(player_pos->y, player_pos->x, PLAYER);

  if (player_can_sense_monsters())
    player_add_sense_monsters(true);
  if (player_is_hallucinating())
    daemon_change_visuals(0);
}

char
level_get_type(int y, int x)
{
  monster* monster = level_get_monster(y, x);
  return monster == nullptr
    ? level_get_ch(y, x)
    : monster->t_disguise;
}

PLACE*
level_get_place(int y, int x)
{
  return &level_places[((x) << 5) + (y)];
}

monster*
level_get_monster(int y, int x)
{
  return level_places[(x << 5) + y].p_monst;
}

void
level_set_monster(int y, int x, monster* monster)
{
  level_places[(x << 5) + y].p_monst = monster;
}

char
level_get_flags(int y, int x)
{
  return level_places[(x << 5) + y].p_flags;
}

void
level_set_flags(int y, int x, char flags)
{
  level_places[(x << 5) + y].p_flags = flags;
}

char level_get_ch(int y, int x)
{
  return level_places[(x << 5) + y].p_ch;
}

void level_set_ch(int y, int x, char ch)
{
  level_places[(x << 5) + y].p_ch = ch;
}
