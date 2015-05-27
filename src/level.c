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

#include "traps.h"
#include "io.h"
#include "pack.h"
#include "daemons.h"
#include "list.h"
#include "monster.h"
#include "passages.h"
#include "misc.h"
#include "player.h"
#include "rooms.h"
#include "things.h"
#include "os.h"
#include "state.h"
#include "rogue.h"

#include "level.h"

/* Tuneables */
#define MAXOBJ		9  /* How many attempts to put items in dungeon */
#define TREAS_ROOM	20 /* one chance in TREAS_ROOM for a treasure room */
#define MAXTREAS	10 /* maximum number of treasures in a treasure room */
#define MINTREAS	2  /* minimum number of treasures in a treasure room */
#define MAXTRIES	10 /* max number of tries to put down a monster */
#define MAXTRAPS	10

PLACE places[MAXLINES*MAXCOLS];
THING* lvl_obj = NULL;
coord stairs;
int level = 1;
int max_level = 1;


/** treas_room:
 * Add a treasure room */
static void
treas_room(void)
{
  struct room* room = &rooms[room_random()];
  int spots = (room->r_max.y - 2) * (room->r_max.x - 2) - MINTREAS;

  if (spots > (MAXTREAS - MINTREAS))
    spots = (MAXTREAS - MINTREAS);
  int num_monsters = rnd(spots) + MINTREAS;

  for (int i = 0; i < num_monsters; ++i)
  {
    coord monster_pos;
    THING* monster = new_thing();

    room_find_floor(room, &monster_pos, 2 * MAXTRIES, false);
    monster->o_pos = monster_pos;
    list_attach(&lvl_obj, monster);
    level_set_ch(monster_pos.y, monster_pos.x, (char)monster->o_type);
  }

  /* fill up room with monsters from the next level down */
  int nm = rnd(spots) + MINTREAS;
  if (nm < num_monsters + 2)
    nm = num_monsters + 2;

  spots = (room->r_max.y - 2) * (room->r_max.x - 2);
  if (nm > spots)
    nm = spots;
  level++;
  while (nm--)
  {
    coord monster_pos;
    spots = 0;
    if (room_find_floor(room, &monster_pos, MAXTRIES, true))
    {
      THING* tp = allocate_new_item();
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
  if (pack_contains_amulet() && level < max_level)
      return;

  /* check for treasure rooms, and if so, put it in. */
  if (rnd(TREAS_ROOM) == 0)
    treas_room();

  /* Do MAXOBJ attempts to put things on a level */
  for (i = 0; i < MAXOBJ; i++)
    if (rnd(100) < 36)
    {
      /* Pick a new object and link it in the list */
      THING* obj = new_thing();
      list_attach(&lvl_obj, obj);

      /* Put it somewhere */
      room_find_floor((struct room *) NULL, &obj->o_pos, false, false);
      level_set_ch(obj->o_pos.y, obj->o_pos.x, (char)obj->o_type);
    }

  /* If he is really deep in the dungeon and he hasn't found the
   * amulet yet, put it somewhere on the ground */
  if (level >= AMULETLEVEL && !pack_contains_amulet())
  {
    THING* obj = allocate_new_item();
    list_attach(&lvl_obj, obj);
    obj->o_hplus = 0;
    obj->o_dplus = 0;
    strncpy(obj->o_damage,"0x0",sizeof(obj->o_damage));
    strncpy(obj->o_hurldmg,"0x0",sizeof(obj->o_hurldmg));
    obj->o_arm = 11;
    obj->o_type = AMULET;

    /* Put it somewhere */
    room_find_floor((struct room *) NULL, &obj->o_pos, false, false);
    level_set_ch(obj->o_pos.y, obj->o_pos.x, AMULET);
  }
}


void
level_new(void)
{
  /* unhold when you go down just in case */
  player_remove_held();

  /* Set max level we've been to */
  if (level > max_level)
    max_level = level;

  /* Clean things off from last level */
  for (PLACE* pp = places; pp < &places[MAXCOLS*MAXLINES]; pp++)
  {
      pp->p_ch = SHADOW;
      pp->p_flags = F_REAL;
      pp->p_monst = NULL;
  }
  clear();

  /* Free up the monsters on the last level */
  for (THING* monster = mlist; monster != NULL; monster = monster->l_next)
    list_free_all(&monster->t_pack);
  list_free_all(&mlist);

  /* Throw away stuff left on the previous level (if anything) */
  list_free_all(&lvl_obj);

  rooms_create(); /* Draw rooms */
  passages_do();  /* Draw passages */
  no_food++;      /* Levels with no food placed */
  put_things();   /* Place objects (if any) */

  /* Place the traps */
  if (rnd(10) < level)
  {
    int ntraps = rnd(level / 4) + 1;
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
        room_find_floor((struct room *) NULL, &stairs, false, false);
      while (level_get_ch(stairs.y, stairs.x) != FLOOR);

      char trapflag = level_get_flags(stairs.y, stairs.x);
      trapflag &= ~F_REAL;
      trapflag |= rnd(NTRAPS);
      level_set_flags(stairs.y, stairs.x, trapflag);
    }
  }

  /* Place the staircase down.  */
  room_find_floor((struct room *) NULL, &stairs, false, false);
  level_set_ch(stairs.y, stairs.x, STAIRS);

  for (THING* monster = mlist; monster != NULL; monster = monster->l_next)
    monster->t_room = roomin(&monster->t_pos);

  coord* player_pos = player_get_pos();
  room_find_floor((struct room *) NULL, player_pos, false, true);
  room_enter(player_pos);
  mvaddcch(player_pos->y, player_pos->x, PLAYER);

  if (player_can_sense_monsters())
    player_add_sense_monsters(true);
  if (player_is_hallucinating())
    daemon_change_visuals();
}

bool
level_save_state(void)
{
  return state_save_list(lvl_obj)
    || state_save_int32(level)
    || state_save_int32(max_level)
    || state_save_coord(&stairs);
}

bool
level_load_state(void)
{
  return state_load_list(&lvl_obj)
    || state_load_int32(&level)
    || state_load_int32(&max_level)
    || state_load_coord(&stairs);
}

char
level_get_type(int y, int x)
{
  THING* monster = level_get_monster(y, x);
  return monster == NULL
    ? level_get_ch(y, x)
    : monster->t_disguise;
}

THING*
level_get_monster(int y, int x)
{
  return places[(x << 5) + y].p_monst;
}

void
level_set_monster(int y, int x, THING* monster)
{
  places[(x << 5) + y].p_monst = monster;
}

char
level_get_flags(int y, int x)
{
  return places[(x << 5) + y].p_flags;
}

void
level_set_flags(int y, int x, char flags)
{
  places[(x << 5) + y].p_flags = flags;
}

char level_get_ch(int y, int x)
{
  return places[(x << 5) + y].p_ch;
}

void level_set_ch(int y, int x, char ch)
{
  places[(x << 5) + y].p_ch = ch;
}
