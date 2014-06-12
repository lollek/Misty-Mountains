/*
 * Function(s) for dealing with potions
 *
 * @(#)potions.c	4.46 (Berkeley) 06/07/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <ctype.h>

#include "potions.h"
#include "rogue.h"

struct obj_info pot_info[NPOTIONS] = {
  /* io_name,      oi_prob, oi_worth, oi_guess, oi_know */
  { "confusion",         7,        5,     NULL, FALSE },
  { "hallucination",     8,        5,     NULL, FALSE },
  { "poison",            8,        5,     NULL, FALSE },
  { "gain strength",    13,      150,     NULL, FALSE },
  { "see invisible",     3,      100,     NULL, FALSE },
  { "healing",          13,      130,     NULL, FALSE },
  { "monster detection", 6,      130,     NULL, FALSE },
  { "magic detection",   6,      105,     NULL, FALSE },
  { "raise level",       2,      250,     NULL, FALSE },
  { "extra healing",     5,      200,     NULL, FALSE },
  { "haste self",        5,      190,     NULL, FALSE },
  { "restore strength", 13,      130,     NULL, FALSE },
  { "blindness",         5,        5,     NULL, FALSE },
  { "levitation",        6,       75,     NULL, FALSE },
};

static PACT p_actions[] =
{
	{ ISHUH,	unconfuse,	HUHDURATION,	/* P_CONFUSE */
		"what a tripy feeling!",
		"wait, what's going on here. Huh? What? Who?" },
	{ ISHALU,	come_down,	SEEDURATION,	/* P_LSD */
		"Oh, wow!  Everything seems so cosmic!",
		"Oh, wow!  Everything seems so cosmic!" },
	{ 0,		NULL,	0,	NULL,	NULL },	/* P_POISON */
	{ 0,		NULL,	0,	NULL,	NULL },	/* P_STRENGTH */
	{ CANSEE,	unsee,	SEEDURATION,		/* P_SEEINVIS */
		prbuf,
		prbuf },
	{ 0,		NULL,	0,	NULL,	NULL },	/* P_HEALING */
	{ 0,		NULL,	0,	NULL,	NULL },	/* P_MFIND */
	{ 0,		NULL,	0,	NULL,	NULL },	/* P_TFIND  */
	{ 0,		NULL,	0,	NULL,	NULL },	/* P_RAISE */
	{ 0,		NULL,	0,	NULL,	NULL },	/* P_XHEAL */
	{ 0,		NULL,	0,	NULL,	NULL },	/* P_HASTE */
	{ 0,		NULL,	0,	NULL,	NULL },	/* P_RESTORE */
	{ ISBLIND,	sight,	SEEDURATION,		/* P_BLIND */
		"oh, bummer!  Everything is dark!  Help!",
		"a cloak of darkness falls around you" },
	{ ISLEVIT,	land,	HEALTIME,		/* P_LEVIT */
		"oh, wow!  You're floating in the air!",
		"you start to float in the air" }
};

void
quaff()
{
    THING *obj, *tp, *mp;
    bool discardit = FALSE;
    bool show, trip;

    obj = get_item("quaff", POTION);
    /*
     * Make certain that it is somethings that we want to drink
     */
    if (obj == NULL)
	return;
    if (obj->o_type != POTION)
    {
	if (!terse)
	    msg("yuk! Why would you want to drink that?");
	else
	    msg("that's undrinkable");
	return;
    }
    if (obj == cur_weapon)
	cur_weapon = NULL;

    /*
     * Calculate the effect it has on the poor guy.
     */
    trip = on(player, ISHALU);
    discardit = (bool)(obj->o_count == 1);
    leave_pack(obj, FALSE, FALSE);
    switch (obj->o_which)
    {
	case P_CONFUSE:
	    do_pot(P_CONFUSE, !trip);
	when P_POISON:
	    pot_info[P_POISON].oi_know = TRUE;
	    if (ISWEARING(R_SUSTSTR))
		msg("you feel momentarily sick");
	    else
	    {
		chg_str(-(rnd(3) + 1));
		msg("you feel very sick now");
		come_down();
	    }
	when P_HEALING:
	    pot_info[P_HEALING].oi_know = TRUE;
	    if ((pstats.s_hpt += roll(pstats.s_lvl, 4)) > max_hp)
		pstats.s_hpt = ++max_hp;
	    sight();
	    msg("you begin to feel better");
	when P_STRENGTH:
	    pot_info[P_STRENGTH].oi_know = TRUE;
	    chg_str(1);
	    msg("you feel stronger, now.  What bulging muscles!");
	when P_MFIND:
	    player.t_flags |= SEEMONST;
	    fuse((void(*)())turn_see, TRUE, HUHDURATION, AFTER);
	    if (!turn_see(FALSE))
		msg("you have a %s feeling for a moment, then it passes",
		    choose_str("normal", "strange"));
	when P_TFIND:
	    /*
	     * Potion of magic detection.  Show the potions and scrolls
	     */
	    show = FALSE;
	    if (lvl_obj != NULL)
	    {
		wclear(hw);
		for (tp = lvl_obj; tp != NULL; tp = next(tp))
		{
		    if (is_magic(tp))
		    {
			show = TRUE;
			wmove(hw, tp->o_pos.y, tp->o_pos.x);
			waddcch(hw, MAGIC);
			pot_info[P_TFIND].oi_know = TRUE;
		    }
		}
		for (mp = mlist; mp != NULL; mp = next(mp))
		{
		    for (tp = mp->t_pack; tp != NULL; tp = next(tp))
		    {
			if (is_magic(tp))
			{
			    show = TRUE;
			    wmove(hw, mp->t_pos.y, mp->t_pos.x);
			    waddcch(hw, MAGIC);
			}
		    }
		}
	    }
	    if (show)
	    {
		pot_info[P_TFIND].oi_know = TRUE;
		show_win("You sense the presence of magic on this level.--More--");
	    }
	    else
		msg("you have a %s feeling for a moment, then it passes",
		    choose_str("normal", "strange"));
	when P_LSD:
	    if (!trip)
	    {
		if (on(player, SEEMONST))
		    turn_see(FALSE);
		start_daemon(visuals, 0, BEFORE);
		seenstairs = seen_stairs();
	    }
	    do_pot(P_LSD, TRUE);
	when P_SEEINVIS:
	    sprintf(prbuf, "this potion tastes like fruit juice");
	    show = on(player, CANSEE);
	    do_pot(P_SEEINVIS, FALSE);
	    if (!show)
		invis_on();
	    sight();
	when P_RAISE:
	    pot_info[P_RAISE].oi_know = TRUE;
	    msg("you suddenly feel much more skillful");
	    raise_level();
	when P_XHEAL:
	    pot_info[P_XHEAL].oi_know = TRUE;
	    if ((pstats.s_hpt += roll(pstats.s_lvl, 8)) > max_hp)
	    {
		if (pstats.s_hpt > max_hp + pstats.s_lvl + 1)
		    ++max_hp;
		pstats.s_hpt = ++max_hp;
	    }
	    sight();
	    come_down();
	    msg("you begin to feel much better");
	when P_HASTE:
	    pot_info[P_HASTE].oi_know = TRUE;
	    after = FALSE;
	    if (add_haste(TRUE))
		msg("you feel yourself moving much faster");
	when P_RESTORE:
	    if (ISRING(LEFT, R_ADDSTR))
		add_str(&pstats.s_str, -cur_ring[LEFT]->o_arm);
	    if (ISRING(RIGHT, R_ADDSTR))
		add_str(&pstats.s_str, -cur_ring[RIGHT]->o_arm);
	    if (pstats.s_str < max_stats.s_str)
		pstats.s_str = max_stats.s_str;
	    if (ISRING(LEFT, R_ADDSTR))
		add_str(&pstats.s_str, cur_ring[LEFT]->o_arm);
	    if (ISRING(RIGHT, R_ADDSTR))
		add_str(&pstats.s_str, cur_ring[RIGHT]->o_arm);
	    msg("hey, this tastes great.  It make you feel warm all over");
	when P_BLIND:
	    do_pot(P_BLIND, TRUE);
	when P_LEVIT:
	    do_pot(P_LEVIT, TRUE);
	otherwise:
            if (wizard)
            {
              msg("what an odd tasting potion!");
              return;
            }
    }
    status();
    /*
     * Throw the item away
     */

    call_it(&pot_info[obj->o_which]);

    if (discardit)
	discard(obj);
    return;
}


void
do_pot(int type, bool knowit)
{
    PACT *pp;
    int t;

    pp = &p_actions[type];
    if (!pot_info[type].oi_know)
	pot_info[type].oi_know = knowit;
    t = spread(pp->pa_time);
    if (!on(player, pp->pa_flags))
    {
	player.t_flags |= pp->pa_flags;
	fuse(pp->pa_daemon, 0, t, AFTER);
	look(FALSE);
    }
    else
	lengthen(pp->pa_daemon, t);
    msg(choose_str(pp->pa_high, pp->pa_straight));
}
