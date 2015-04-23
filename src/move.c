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

#include "rogue.h"
#include "status_effects.h"
#include "scrolls.h"
#include "command.h"
#include "traps.h"
#include "io.h"
#include "chase.h"
#include "armor.h"
#include "pack.h"

/*
 * do_run:
 *	Start the hero running
 */

bool
do_run(char ch, bool cautiously)
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

/*
 * do_move:
 *	Check to see that a move is legal.  If it is handle the
 * consequences (fighting, picking up, etc.)
 */

bool
do_move(char ch)
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
	nh = *rndmove(&player);
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
			b1 = (bool)(hero.y != 1 && turn_ok(hero.y - 1, hero.x));
			b2 = (bool)(hero.y != NUMLINES - 2 && turn_ok(hero.y + 1, hero.x));
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
			turnref();
			goto over;
		    case 'j':
		    case 'k':
			b1 = (bool)(hero.x != 0 && turn_ok(hero.y, hero.x - 1));
			b2 = (bool)(hero.x != NUMCOLS - 1 && turn_ok(hero.y, hero.x + 1));
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
			turnref();
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
		fight(&nh, equipped_item(EQUIPMENT_RHAND), false);
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

/*
 * turn_ok:
 *	Decide whether it is legal to turn onto the given space
 */
bool
turn_ok(int y, int x)
{
    PLACE *pp;

    pp = INDEX(y, x);
    return (pp->p_ch == DOOR
	|| (pp->p_flags & (F_REAL|F_PASS)) == (F_REAL|F_PASS));
}

/*
 * turnref:
 *	Decide whether to refresh at a passage turning or not
 */

void
turnref(void)
{
    PLACE *pp;

    pp = INDEX(hero.y, hero.x);
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

/*
 * door_open:
 *	Called to illuminate a room.  If it is dark, remove anything
 *	that might move.
 */

void
door_open(struct room *rp)
{
    int y, x;

    if (!(rp->r_flags & ISGONE))
	for (y = rp->r_pos.y; y < rp->r_pos.y + rp->r_max.y; y++)
	    for (x = rp->r_pos.x; x < rp->r_pos.x + rp->r_max.x; x++)
		if (isupper(winat(y, x)))
		    wake_monster(y, x);
}

/* rndmove:
 * Move in a random direction if the monster/person is confused */
coord *
rndmove(THING *who)
{
    THING *obj;
    int x, y;
    char ch;
    static coord ret;  /* what we will be returning */

    y = ret.y = who->t_pos.y + rnd(3) - 1;
    x = ret.x = who->t_pos.x + rnd(3) - 1;
    /*
     * Now check to see if that's a legal move.  If not, don't move.
     * (I.e., bump into the wall or whatever)
     */
    if (y == who->t_pos.y && x == who->t_pos.x)
	return &ret;
    if (!diag_ok(&who->t_pos, &ret))
	goto bad;
    else
    {
	ch = winat(y, x);
	if (!step_ok(ch))
	    goto bad;
	if (ch == SCROLL)
	{
	    for (obj = lvl_obj; obj != NULL; obj = obj->l_next)
		if (y == obj->o_pos.y && x == obj->o_pos.x)
		    break;
	    if (obj != NULL && obj->o_which == S_SCARE)
		goto bad;
	}
    }
    return &ret;

bad:
    ret = who->t_pos;
    return &ret;
}

