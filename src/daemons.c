/*
 * All the daemon and fuse functions are in here
 *
 * @(#)daemons.c	4.24 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include "rogue.h"
#include "status_effects.h"
#include "command.h"
#include "io.h"
#include "chase.h"
#include "pack.h"

/*
 * doctor:
 *	A healing daemon that restors hit points after rest
 */
void
doctor(void)
{
    int lv = pstats.s_lvl;
    int ohp = pstats.s_hpt;

    if (ohp == max_hp)
      return;

    quiet++;
    if (lv < 8)
    {
	if (quiet + (lv << 1) > 20)
	    pstats.s_hpt++;
    }
    else
	if (quiet >= 3)
	    pstats.s_hpt += rnd(lv - 7) + 1;
    if (ISRING(LEFT, R_REGEN))
	pstats.s_hpt++;
    if (ISRING(RIGHT, R_REGEN))
	pstats.s_hpt++;
    if (ohp != pstats.s_hpt)
      quiet = 0;

    if (pstats.s_hpt >= max_hp)
    {
	pstats.s_hpt = max_hp;
	stop_counting(false);
    }
}

/*
 * Swander:
 *	Called when it is time to start rolling for wandering monsters
 */
void
swander(void)
{
    start_daemon(rollwand, 0, BEFORE);
}

/*
 * rollwand:
 *	Called to roll to see if a wandering monster starts up
 */
int between = 0;
void
rollwand(void)
{

    if (++between >= 4)
    {
	if (roll(1, 6) == 4)
	{
	    wanderer();
	    kill_daemon(rollwand);
	    fuse(swander, 0, WANDERTIME, BEFORE);
	}
	between = 0;
    }
}

/*
 * stomach:
 *	Digest the hero's food
 */
void
stomach(void)
{
    int oldfood;
    int orig_hungry = hungry_state;

    if (food_left <= 0)
    {
	if (food_left-- < -STARVETIME)
	    death('s');
	/*
	 * the hero is fainting
	 */
	if (no_command || rnd(5) != 0)
	    return;
	no_command += rnd(8) + 4;
	hungry_state = 3;
	if (!terse)
	    addmsg(is_hallucinating(&player)
		? "the munchies overpower your motor capabilities.  "
		: "you feel too weak from lack of food.  ");
	msg(is_hallucinating(&player)
	    ? "You freak out"
	    : "You faint");
    }
    else
    {
	oldfood = food_left;
	food_left -= ring_eat(LEFT) + ring_eat(RIGHT) + 1 - player_has_amulet();

	if (food_left < MORETIME && oldfood >= MORETIME)
	{
	    hungry_state = 2;
	    msg(is_hallucinating(&player)
		? "the munchies are interfering with your motor capabilites"
		: "you are starting to feel weak");
	}
	else if (food_left < 2 * MORETIME && oldfood >= 2 * MORETIME)
	{
	    hungry_state = 1;
	    if (terse)
		msg(is_hallucinating(&player)
		    ? "getting the munchies"
		    : "getting hungry");
	    else
		msg(is_hallucinating(&player)
		    ? "you are getting the munchies"
		    : "you are starting to get hungry");
	}
    }
    if (hungry_state != orig_hungry)
	stop_counting(true);
}

/*
 * visuals:
 *	change the characters for the player
 */
void
visuals(void)
{
    THING *tp;
    bool seemonst;

    if (!after || (running && jump))
	return;
    /*
     * change the things
     */
    for (tp = lvl_obj; tp != NULL; tp = tp->l_next)
	if (cansee(tp->o_pos.y, tp->o_pos.x))
	    mvaddcch(tp->o_pos.y, tp->o_pos.x, rnd_thing());

    /*
     * change the stairs
     */
    if (!seen_stairs())
	mvaddcch(stairs.y, stairs.x, rnd_thing());

    /*
     * change the monsters
     */
    seemonst = on(player, SEEMONST);
    for (tp = mlist; tp != NULL; tp = tp->l_next)
    {
	move(tp->t_pos.y, tp->t_pos.x);
	if (see_monst(tp))
	{
	    if (tp->t_type == 'X' && tp->t_disguise != 'X')
		addcch(rnd_thing());
	    else
		addcch(rnd(26) + 'A');
	}
	else if (seemonst)
	    addcch((rnd(26) + 'A') | A_STANDOUT);
    }
}
