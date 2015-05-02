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

#include "scrolls.h"
#include "command.h"
#include "traps.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "fight.h"
#include "monster.h"
#include "move.h"
#include "rooms.h"
#include "misc.h"
#include "level.h"
#include "player.h"
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
  coord *player_pos = player_get_pos();
  PLACE *pp = INDEX(player_pos->y, player_pos->x);

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
	case 'h': dy =  0; dx = -1; break;
	case 'j': dy =  1; dx =  0; break;
	case 'k': dy = -1; dx =  0; break;
	case 'l': dy =  0, dx =  1; break;
	case 'y': dy = -1, dx = -1; break;
	case 'u': dy = -1, dx =  1; break;
	case 'b': dy =  1, dx = -1; break;
	case 'n': dy =  1, dx =  1; break;
    }

    firstmove = false;
    if (no_move)
    {
	no_move--;
	msg("you are still stuck in the bear trap");
	return true;
    }

    if (player_is_confused() && rnd(5) != 0)
    {
      /* TODO: Remove __player_ptr() */
	move_random(__player_ptr(), &nh);
	if (same_coords(nh, *player_get_pos()))
	{
	    running = false;
	    to_death = false;
	    return false;
	}
    }
    else
    {
over:
      {
	coord *player_pos = player_get_pos();
	nh.y = player_pos->y + dy;
	nh.x = player_pos->x + dx;
      }
    }

    /* Check if he tried to move off the screen or make an illegal
     * diagonal move, and stop him if he did. */
    if (nh.x < 0 || nh.x >= NUMCOLS || nh.y <= 0 || nh.y >= NUMLINES - 1)
	goto hit_bound;

    if (!diag_ok(player_get_pos(), &nh))
    {
	running = false;
	return false;
    }

    if (running && same_coords(*player_get_pos(), nh))
	after = running = false;

    fl = flat(nh.y, nh.x);
    ch = winat(nh.y, nh.x);

    if (!(fl & F_REAL) && ch == FLOOR)
    {
	if (!player_is_levitating())
	{
	    chat(nh.y, nh.x) = ch = TRAP;
	    flat(nh.y, nh.x) |= F_REAL;
	}
    }
    else if (player_is_held() && ch != 'F')
    {
	msg("you are being held");
	return after;
    }

    switch (ch)
    {
	case SHADOW: case VWALL: case HWALL:
hit_bound:
	    if (passgo && running && (player_get_room()->r_flags & ISGONE)
		&& !player_is_blind())
	    {
		coord *player_pos = player_get_pos();
		bool b1, b2;

		switch (runch)
		{
		    case 'h':
		    case 'l':
			b1 = (bool)(player_pos->y != 1 && move_turn_ok(player_pos->y - 1, player_pos->x));
			b2 = (bool)(player_pos->y != NUMLINES - 2 && move_turn_ok(player_pos->y + 1, player_pos->x));
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
			b1 = (bool)(player_pos->x != 0 && move_turn_ok(player_pos->y, player_pos->x - 1));
			b2 = (bool)(player_pos->x != NUMCOLS - 1 && move_turn_ok(player_pos->y, player_pos->x + 1));
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
            break;
	case DOOR:
          {
            coord *player_pos = player_get_pos();
	    running = false;
	    if (flat(player_pos->y, player_pos->x) & F_PASS)
		room_enter(&nh);
	    mvaddcch(player_pos->y, player_pos->x, floor_at());
	    if ((fl & F_PASS) && chat(oldpos.y, oldpos.x) == DOOR)
		room_leave(&nh);
            player_set_pos(&nh);
          }
            break;
	case TRAP:
          {
            coord *player_pos = player_get_pos();
	    ch = be_trapped(&nh);
	    if (ch == T_DOOR || ch == T_TELEP)
		return after;
	    mvaddcch(player_pos->y, player_pos->x, floor_at());
	    if ((fl & F_PASS) && chat(oldpos.y, oldpos.x) == DOOR)
		room_leave(&nh);
            player_set_pos(&nh);
          }
          break;
	case PASSAGE:
          {
	    /*
	     * when you're in a corridor, you don't know if you're in
	     * a maze room or not, and there ain't no way to find out
	     * if you're leaving a maze room, so it is necessary to
	     * always recalculate proom.
	     */
            coord *player_pos = player_get_pos();
	    player_set_room(roomin(player_pos));
	    mvaddcch(player_pos->y, player_pos->x, floor_at());
	    if ((fl & F_PASS) && chat(oldpos.y, oldpos.x) == DOOR)
		room_leave(&nh);
            player_set_pos(&nh);
          }
          break;
	case FLOOR:
          {
            coord *player_pos = player_get_pos();
	    if (!(fl & F_REAL))
		be_trapped(player_pos);
		mvaddcch(player_pos->y, player_pos->x, floor_at());
		if ((fl & F_PASS) && chat(oldpos.y, oldpos.x) == DOOR)
		    room_leave(&nh);
                player_set_pos(&nh);
          }
          break;
	default:
	    running = false;
	    if (isupper(ch) || moat(nh.y, nh.x))
		fight_against_monster(&nh, pack_equipped_item(EQUIPMENT_RHAND), false);
	    else
	    {
              coord *player_pos = player_get_pos();
		if (ch != STAIRS)
		    take = ch;
		mvaddcch(player_pos->y, player_pos->x, floor_at());
		if ((fl & F_PASS) && chat(oldpos.y, oldpos.x) == DOOR)
		    room_leave(&nh);
                player_set_pos(&nh);
	    }
            break;
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

