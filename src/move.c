/*
 * hero movement commands
 *
 * @(#)move.c	4.49 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <ctype.h>
#include <assert.h>

#include "status_effects.h"
#include "scrolls.h"
#include "command.h"
#include "traps.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "fight.h"
#include "monsters.h"
#include "move.h"
#include "rogue.h"

/** move_turn_ok:
 * Decide whether it is legal to turn onto the given space */
static bool
move_turn_ok(int y, int x)
{
  PLACE *pp = INDEX(y, x);
  return (pp->p_ch == DOOR
      || (pp->p_flags & (F_REAL|F_PASS)) == (F_REAL|F_PASS));
}

/** move_turnref:
 * Decide whether to refresh at a passage turning or not */
static void
move_turnref(void)
{
  PLACE *pp = INDEX(hero.y, hero.x);

  if (!(pp->p_flags & F_SEEN))
  {
    if (jump)
    {
      leaveok(stdscr, true);
      refresh();
      leaveok(stdscr, false);
    }
    pp->p_flags |= F_SEEN;
  }
}

bool
move_do_run(char ch, bool cautiously)
{
  if (cautiously)
  {
    door_stop = true;
    firstmove = true;
  }

  running = true;
  runch = tolower(ch);
  return false;
}

/* TODO: Clean up this monster */
bool
move_do(char ch)
{
    int dy = 0, dx = 0;
    char fl;
    coord nh;

    switch (ch)
    {
	case 'h': dy =  0; dx = -1;
	when 'j': dy =  1; dx =  0;
	when 'k': dy = -1; dx =  0;
	when 'l': dy =  0, dx =  1;
	when 'y': dy = -1, dx = -1;
	when 'u': dy = -1, dx =  1;
	when 'b': dy =  1, dx = -1;
	when 'n': dy =  1, dx =  1;
    }

    firstmove = false;
    if (no_move)
    {
	no_move--;
	msg("you are still stuck in the bear trap");
	return true;
    }

    if (is_confused(&player) && rnd(5) != 0)
    {
	move_random(&player, &nh);
	if (same_coords(nh, hero))
	{
	    running = false;
	    to_death = false;
	    return false;
	}
    }
    else
    {
over:
	nh.y = hero.y + dy;
	nh.x = hero.x + dx;
    }

    /* Check if he tried to move off the screen or make an illegal
     * diagonal move, and stop him if he did. */
    if (nh.x < 0 || nh.x >= NUMCOLS || nh.y <= 0 || nh.y >= NUMLINES - 1)
	goto hit_bound;

    if (!diag_ok(&hero, &nh))
    {
	running = false;
	return false;
    }

    if (running && same_coords(hero, nh))
	after = running = false;

    fl = flat(nh.y, nh.x);
    ch = winat(nh.y, nh.x);

    if (!(fl & F_REAL) && ch == FLOOR)
    {
	if (!is_levitating(&player))
	{
	    chat(nh.y, nh.x) = ch = TRAP;
	    flat(nh.y, nh.x) |= F_REAL;
	}
    }
    else if (on(player, ISHELD) && ch != 'F')
    {
	msg("you are being held");
	return after;
    }

    switch (ch)
    {
	case SHADOW: case VWALL: case HWALL:
hit_bound:
	    if (passgo && running && (proom->r_flags & ISGONE)
		&& !is_blind(&player))
	    {
		bool b1, b2;

		switch (runch)
		{
		    case 'h':
		    case 'l':
			b1 = (bool)(hero.y != 1 && move_turn_ok(hero.y - 1, hero.x));
			b2 = (bool)(hero.y != NUMLINES - 2 && move_turn_ok(hero.y + 1, hero.x));
			if (!(b1 ^ b2))
			    break;
			if (b1)
			{
			    runch = 'k';
			    dy = -1;
			}
			else
			{
			    runch = 'j';
			    dy = 1;
			}
			dx = 0;
			move_turnref();
			goto over;
		    case 'j':
		    case 'k':
			b1 = (bool)(hero.x != 0 && move_turn_ok(hero.y, hero.x - 1));
			b2 = (bool)(hero.x != NUMCOLS - 1 && move_turn_ok(hero.y, hero.x + 1));
			if (!(b1 ^ b2))
			    break;
			if (b1)
			{
			    runch = 'h';
			    dx = -1;
			}
			else
			{
			    runch = 'l';
			    dx = 1;
			}
			dy = 0;
			move_turnref();
			goto over;
		}
	    }
	    running = false;
	    after = false;
	when DOOR:
	    running = false;
	    if (flat(hero.y, hero.x) & F_PASS)
		enter_room(&nh);
	    mvaddcch(hero.y, hero.x, floor_at());
	    if ((fl & F_PASS) && chat(oldpos.y, oldpos.x) == DOOR)
		leave_room(&nh);
	    hero = nh;
	when TRAP:
	    ch = be_trapped(&player, &nh);
	    if (ch == T_DOOR || ch == T_TELEP)
		return after;
	    mvaddcch(hero.y, hero.x, floor_at());
	    if ((fl & F_PASS) && chat(oldpos.y, oldpos.x) == DOOR)
		leave_room(&nh);
	    hero = nh;
	when PASSAGE:
	    /*
	     * when you're in a corridor, you don't know if you're in
	     * a maze room or not, and there ain't no way to find out
	     * if you're leaving a maze room, so it is necessary to
	     * always recalculate proom.
	     */
	    proom = roomin(&hero);
	    mvaddcch(hero.y, hero.x, floor_at());
	    if ((fl & F_PASS) && chat(oldpos.y, oldpos.x) == DOOR)
		leave_room(&nh);
	    hero = nh;
	when FLOOR:
	    if (!(fl & F_REAL))
		be_trapped(&player, &hero);
		mvaddcch(hero.y, hero.x, floor_at());
		if ((fl & F_PASS) && chat(oldpos.y, oldpos.x) == DOOR)
		    leave_room(&nh);
		hero = nh;
	otherwise:
	    running = false;
	    if (isupper(ch) || moat(nh.y, nh.x))
		fight_against_monster(&nh, pack_equipped_item(EQUIPMENT_RHAND), false);
	    else
	    {
		if (ch != STAIRS)
		    take = ch;
		mvaddcch(hero.y, hero.x, floor_at());
		if ((fl & F_PASS) && chat(oldpos.y, oldpos.x) == DOOR)
		    leave_room(&nh);
		hero = nh;
	    }
    }
    return after;
}

/** move_random:
 * Move in a random direction if the monster/person is confused */
void
move_random(THING *who, coord *ret)
{
  THING *obj;
  int x = ret->x = who->t_pos.x + rnd(3) - 1;
  int y = ret->y = who->t_pos.y + rnd(3) - 1;
  char ch;

  assert(who != NULL);

  /* Now check to see if that's a legal move.
   * If not, don't move.(I.e., bump into the wall or whatever) */
  if (y == who->t_pos.y && x == who->t_pos.x)
    return;

  if (!diag_ok(&who->t_pos, ret))
  {
    ret->x = who->t_pos.x;
    ret->y = who->t_pos.y;
    return;
  }

  ch = winat(y, x);
  if (!step_ok(ch))
  {
    ret->x = who->t_pos.x;
    ret->y = who->t_pos.y;
    return;
  }

  if (ch == SCROLL)
  {
    for (obj = lvl_obj; obj != NULL; obj = obj->l_next)
      if (y == obj->o_pos.y && x == obj->o_pos.x)
        break;

    if (obj != NULL && obj->o_which == S_SCARE)
    {
      ret->x = who->t_pos.x;
      ret->y = who->t_pos.y;
      return;
    }
  }
}

