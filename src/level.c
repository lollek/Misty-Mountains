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

#include "status_effects.h"
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
#include "rogue.h"

#include "level.h"

/* Tuneables */
#define MAXOBJ		9  /* How many attempts to put items in dungeon */
#define TREAS_ROOM	20 /* one chance in TREAS_ROOM for a treasure room */
#define MAXTREAS	10 /* maximum number of treasures in a treasure room */
#define MINTREAS	2  /* minimum number of treasures in a treasure room */
#define MAXTRIES	10 /* max number of tries to put down a monster */
#define MAXTRAPS	10


/** treas_room:
 * Add a treasure room */
static void
treas_room(void)
{
  int nm;
  THING *tp;
  struct room *rp = &rooms[room_random()];
  int spots = (rp->r_max.y - 2) * (rp->r_max.x - 2) - MINTREAS;
  int num_monst;
  static coord mp;

  if (spots > (MAXTREAS - MINTREAS))
    spots = (MAXTREAS - MINTREAS);
  num_monst = nm = rnd(spots) + MINTREAS;

  while (nm--)
  {
    room_find_floor(rp, &mp, 2 * MAXTRIES, false);
    tp = new_thing();
    tp->o_pos = mp;
    attach(lvl_obj, tp);
    chat(mp.y, mp.x) = (char) tp->o_type;
  }

  /* fill up room with monsters from the next level down */
  if ((nm = rnd(spots) + MINTREAS) < num_monst + 2)
    nm = num_monst + 2;
  spots = (rp->r_max.y - 2) * (rp->r_max.x - 2);
  if (nm > spots)
    nm = spots;
  level++;
  while (nm--)
  {
    spots = 0;
    if (room_find_floor(rp, &mp, MAXTRIES, true))
    {
      tp = new_item();
      monster_new(tp, monster_random(false), &mp);
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
      THING *obj = new_thing();
      attach(lvl_obj, obj);

      /* Put it somewhere */
      room_find_floor((struct room *) NULL, &obj->o_pos, false, false);
      chat(obj->o_pos.y, obj->o_pos.x) = (char) obj->o_type;
    }

  /* If he is really deep in the dungeon and he hasn't found the
   * amulet yet, put it somewhere on the ground */
  if (level >= AMULETLEVEL && !pack_contains_amulet())
  {
    THING *obj = new_item();
    attach(lvl_obj, obj);
    obj->o_hplus = 0;
    obj->o_dplus = 0;
    strncpy(obj->o_damage,"0x0",sizeof(obj->o_damage));
    strncpy(obj->o_hurldmg,"0x0",sizeof(obj->o_hurldmg));
    obj->o_arm = 11;
    obj->o_type = AMULET;

    /* Put it somewhere */
    room_find_floor((struct room *) NULL, &obj->o_pos, false, false);
    chat(obj->o_pos.y, obj->o_pos.x) = AMULET;
  }
}


/* TODO: This function needs cleanup */
void
level_new(void)
{
    THING *tp;
    PLACE *pp;
    char *sp;

    player_remove_held(); /* unhold when you go down just in case */
    if (level > max_level)
	max_level = level;
    /*
     * Clean things off from last level
     */
    for (pp = places; pp < &places[MAXCOLS*MAXLINES]; pp++)
    {
	pp->p_ch = SHADOW;
	pp->p_flags = F_REAL;
	pp->p_monst = NULL;
    }
    clear();
    /*
     * Free up the monsters on the last level
     */
    for (tp = mlist; tp != NULL; tp = tp->l_next)
	free_list(tp->t_pack);
    free_list(mlist);
    /*
     * Throw away stuff left on the previous level (if anything)
     */
    free_list(lvl_obj);
    rooms_create();			/* Draw rooms */
    passages_do();			/* Draw passages */
    no_food++;
    put_things();			/* Place objects (if any) */
    /*
     * Place the traps
     */
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
	    {
		room_find_floor((struct room *) NULL, &stairs, false, false);
	    } while (chat(stairs.y, stairs.x) != FLOOR);
	    sp = &flat(stairs.y, stairs.x);
	    *sp &= ~F_REAL;
	    *sp |= rnd(NTRAPS);
	}
    }
    /*
     * Place the staircase down.
     */
    room_find_floor((struct room *) NULL, &stairs, false, false);
    chat(stairs.y, stairs.x) = STAIRS;

    for (tp = mlist; tp != NULL; tp = tp->l_next)
	tp->t_room = roomin(&tp->t_pos);

    {
      coord *player_pos = player_get_pos();
      room_find_floor((struct room *) NULL, player_pos, false, true);
      room_enter(player_pos);
      mvaddcch(player_pos->y, player_pos->x, PLAYER);
    }

    if (player_can_sense_monsters())
	turn_see(false);
    if (player_is_hallucinating())
	daemon_change_visuals();

    if (game_type == QUICK && level > 1 && level <= 20)
      raise_level();
}

