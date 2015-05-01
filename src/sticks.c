/*
 * Functions to implement the various sticks one might find
 * while wandering around the dungeon.
 *
 * @(#)sticks.c	4.39 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <string.h>

#include "status_effects.h"
#include "io.h"
#include "pack.h"
#include "list.h"
#include "monster.h"
#include "rooms.h"
#include "misc.h"
#include "passages.h"
#include "level.h"
#include "player.h"
#include "rogue.h"

struct obj_info ws_info[MAXSTICKS] = {
    { "light",			12, 250, NULL, false },
    { "invisibility",		 6,   5, NULL, false },
    { "lightning",		 3, 330, NULL, false },
    { "fire",			 3, 330, NULL, false },
    { "cold",			 3, 330, NULL, false },
    { "polymorph",		15, 310, NULL, false },
    { "magic missile",		10, 170, NULL, false },
    { "haste monster",		10,   5, NULL, false },
    { "slow monster",		11, 350, NULL, false },
    { "drain life",		 9, 300, NULL, false },
    { "nothing",		 1,   5, NULL, false },
    { "teleport away",		 6, 340, NULL, false },
    { "teleport to",		 6,  50, NULL, false },
    { "cancellation",		 5, 280, NULL, false },
};

/*
 * fix_stick:
 *	Set up a new stick
 */

void
fix_stick(THING *cur)
{
    if (strcmp(ws_type[cur->o_which], "staff") == 0)
	strncpy(cur->o_damage,"2x3",sizeof(cur->o_damage));
    else
	strncpy(cur->o_damage,"1x1",sizeof(cur->o_damage));
    strncpy(cur->o_hurldmg,"1x1",sizeof(cur->o_hurldmg));

    switch (cur->o_which)
    {
	case WS_LIGHT:
	    cur->o_charges = rnd(10) + 10;
            break;
	default:
	    cur->o_charges = rnd(5) + 3;
            break;
    }
}

/*
 * do_zap:
 *	Perform a zap with a wand
 */

bool
do_zap(void)
{
    THING *obj, *tp;
    int y, x;
    char monster, oldch;
    static THING bolt;

    if ((obj = pack_get_item("zap with", STICK)) == NULL)
	return false;
    if (obj->o_type != STICK)
    {
	msg("you can't zap with that!");
	return false;
    }
    if (obj->o_charges == 0)
    {
	msg("nothing happens");
	return true;
    }
    switch (obj->o_which)
    {
	case WS_LIGHT:
	    /*
	     * Reddy Kilowat wand.  Light up the room
	     */
	    ws_info[WS_LIGHT].oi_know = true;
	    if (player_get_room()->r_flags & ISGONE)
		msg("the corridor glows and then fades");
	    else
	    {
		player_get_room()->r_flags &= ~ISDARK;
		/*
		 * Light the room and put the player back up
		 */
		room_enter(player_get_pos());
		addmsg("the room is lit");
		if (!terse)
		    addmsg(" by a shimmering %s light", pick_color("blue"));
		endmsg();
	    }
            break;
	case WS_DRAIN:
	    /*
	     * take away 1/2 of hero's hit points, then take it away
	     * evenly from the monsters in the room (or next to hero
	     * if he is in a passage)
	     */
	    if (player_get_health() < 2)
	    {
		msg("you are too weak to use it");
		return true;
	    }
	    else
		drain();
            break;
	case WS_INVIS:
	case WS_POLYMORPH:
	case WS_TELAWAY:
	case WS_TELTO:
	case WS_CANCEL:
          {
            coord *player_pos = player_get_pos();
	    y = player_pos->y;
	    x = player_pos->x;
	    while (step_ok(winat(y, x)))
	    {
		y += delta.y;
		x += delta.x;
	    }
	    if ((tp = moat(y, x)) != NULL)
	    {
		monster = tp->t_type;
		if (monster == 'F')
		    player.t_flags &= ~ISHELD;
		switch (obj->o_which) {
		    case WS_INVIS:
			set_invisible(tp, true);
			if (cansee(y, x))
			    mvaddcch(y, x, tp->t_oldch);
			break;
		    case WS_POLYMORPH:
		    {
			THING *pp;

			pp = tp->t_pack;
			detach(mlist, tp);
			if (see_monst(tp))
			    mvaddcch(y, x, chat(y, x));
			oldch = tp->t_oldch;
			delta.y = y;
			delta.x = x;
			monster_new(tp, monster = (char)(rnd(26) + 'A'), &delta);
			if (see_monst(tp))
			    mvaddcch(y, x, monster);
			tp->t_oldch = oldch;
			tp->t_pack = pp;
			ws_info[WS_POLYMORPH].oi_know |= see_monst(tp);
			break;
		    }
		    case WS_CANCEL:
			set_cancelled(tp, true);
			set_invisible(tp, false);
			set_confusing(tp, false);
			tp->t_disguise = tp->t_type;
			if (see_monst(tp))
			    mvaddcch(y, x, tp->t_disguise);
			break;
		    case WS_TELAWAY:
		    case WS_TELTO:
                    {
			tp->t_dest = player_pos;
			tp->t_flags |= ISRUN;
			if (obj->o_which == WS_TELTO)
			{
			    coord new_pos;
			    new_pos.y = player_pos->y + delta.y;
			    new_pos.x = player_pos->x + delta.x;
			    teleport(tp, &new_pos);
			}
			else
			    teleport(tp, NULL);
		    }
		}
	    }
          }
          break;
	case WS_MISSILE:
	{
	    THING *weapon = pack_equipped_item(EQUIPMENT_RHAND);
	    ws_info[WS_MISSILE].oi_know = true;
	    bolt.o_type = '*';
	    strncpy(bolt.o_hurldmg,"1x4",sizeof(bolt.o_hurldmg));
	    bolt.o_hplus = 100;
	    bolt.o_dplus = 1;
	    bolt.o_flags = ISMISL;
	    if (weapon != NULL)
		bolt.o_launch = weapon->o_which;
	    do_motion(&bolt, delta.y, delta.x);
	    if ((tp = moat(bolt.o_pos.y, bolt.o_pos.x)) != NULL
		&& !monster_save_throw(VS_MAGIC, tp))
		    hit_monster(bolt.o_pos.y, bolt.o_pos.x, &bolt);
	    else if (terse)
		msg("missle vanishes");
	    else
		msg("the missle vanishes with a puff of smoke");
	}
        break;
	case WS_HASTE_M:
	case WS_SLOW_M:
          {
            coord *player_pos = player_get_pos();
	    y = player_pos->y;
	    x = player_pos->x;
	    while (step_ok(winat(y, x)))
	    {
		y += delta.y;
		x += delta.x;
	    }
	    if ((tp = moat(y, x)) != NULL)
	    {
		if (obj->o_which == WS_HASTE_M)
		{
		    if (on(*tp, ISSLOW))
			tp->t_flags &= ~ISSLOW;
		    else
			tp->t_flags |= ISHASTE;
		}
		else
		{
		    if (on(*tp, ISHASTE))
			tp->t_flags &= ~ISHASTE;
		    else
			tp->t_flags |= ISSLOW;
		    tp->t_turn = true;
		}
		delta.y = y;
		delta.x = x;
		monster_start_running(&delta);
	    }
          }
          break;
	case WS_ELECT:
	case WS_FIRE:
	case WS_COLD:
	{
	    char *name;
	    if (obj->o_which == WS_ELECT)
		name = "bolt";
	    else if (obj->o_which == WS_FIRE)
		name = "flame";
	    else
		name = "ice";
	    fire_bolt(player_get_pos(), &delta, name);
	    ws_info[obj->o_which].oi_know = true;
	}
        break;
	case WS_NOP:
	    break;
	default:
	    msg("what a bizarre schtick!");
            break;
    }
    obj->o_charges--;
    return true;
}

/*
 * drain:
 *	Do drain hit points from player shtick
 */

void
drain(void)
{
    THING *mp;
    struct room *corp;
    THING **dp;
    int cnt;
    bool inpass;
    static THING *drainee[40];
    coord *player_pos = player_get_pos();

    /*
     * First cnt how many things we need to spread the hit points among
     */
    cnt = 0;
    if (chat(player_pos->y, player_pos->x) == DOOR)
	corp = &passages[flat(player_pos->y, player_pos->x) & F_PNUM];
    else
	corp = NULL;
    inpass = (bool)(player_get_room()->r_flags & ISGONE);
    dp = drainee;
    for (mp = mlist; mp != NULL; mp = mp->l_next)
	if (mp->t_room == player_get_room() || mp->t_room == corp ||
	    (inpass && chat(mp->t_pos.y, mp->t_pos.x) == DOOR &&
	    &passages[flat(mp->t_pos.y, mp->t_pos.x) & F_PNUM] == player_get_room()))
		*dp++ = mp;
    if ((cnt = (int)(dp - drainee)) == 0)
    {
	msg("you have a tingling feeling");
	return;
    }
    *dp = NULL;
    player_lose_health(player_get_health() / 2);
    cnt = player_get_health() / cnt;
    /*
     * Now zot all of the monsters
     */
    for (dp = drainee; *dp; dp++)
    {
	mp = *dp;
	if ((mp->t_stats.s_hpt -= cnt) <= 0)
	    monster_on_death(mp, see_monst(mp));
	else
	    monster_start_running(&mp->t_pos);
    }
}

/*
 * fire_bolt:
 *	Fire a bolt in a given direction from a specific starting place
 */

void
fire_bolt(coord *start, coord *dir, char *name)
{
    coord *c1, *c2;
    THING *tp;
    char dirch = 0, ch;
    bool hit_hero, used, changed;
    static coord pos;
    static coord spotpos[BOLT_LENGTH];
    THING bolt;

    bolt.o_type = WEAPON;
    bolt.o_which = FLAME;
    strncpy(bolt.o_hurldmg,"6x6",sizeof(bolt.o_hurldmg));
    bolt.o_hplus = 100;
    bolt.o_dplus = 0;
    weap_info[FLAME].oi_name = name;
    switch (dir->y + dir->x)
    {
	case 0: dirch = '/'; break;
	case 1: case -1: dirch = (dir->y == 0 ? HWALL : VWALL); break;
	case 2: case -2: dirch = '\\'; break;
    }
    pos = *start;
    hit_hero = (bool)(start != player_get_pos());
    used = false;
    changed = false;
    for (c1 = spotpos; c1 <= &spotpos[BOLT_LENGTH-1] && !used; c1++)
    {
	pos.y += dir->y;
	pos.x += dir->x;
	*c1 = pos;
	ch = winat(pos.y, pos.x);
	switch (ch)
	{
	    case DOOR:
		/*
		 * this code is necessary if the hero is on a door
		 * and he fires at the wall the door is in, it would
		 * otherwise loop infinitely
		 */
		if (same_coords(*player_get_pos(), pos))
		    goto def;
		/* FALLTHROUGH */
	    case VWALL: case HWALL: case SHADOW:
		if (!changed)
		    hit_hero = !hit_hero;
		changed = false;
		dir->y = -dir->y;
		dir->x = -dir->x;
		c1--;
		msg("the %s bounces", name);
		break;
	    default:
def:
		if (!hit_hero && (tp = moat(pos.y, pos.x)) != NULL)
		{
		    hit_hero = true;
		    changed = !changed;
		    tp->t_oldch = chat(pos.y, pos.x);
		    if (!monster_save_throw(VS_MAGIC, tp))
		    {
			bolt.o_pos = pos;
			used = true;
			if (tp->t_type == 'D' && strcmp(name, "flame") == 0)
			{
			    addmsg("the flame bounces");
			    if (!terse)
				addmsg(" off the dragon");
			    endmsg();
			}
			else
			    hit_monster(pos.y, pos.x, &bolt);
		    }
		    else if (ch != 'M' || tp->t_disguise == 'M')
		    {
			if (start == player_get_pos())
			    monster_start_running(&pos);
			if (terse)
			    msg("%s misses", name);
			else
			    msg("the %s whizzes past %s", name, set_mname(tp));
		    }
		}
		else if (hit_hero && same_coords(pos, *player_get_pos()))
		{
		    hit_hero = false;
		    changed = !changed;
		    if (!player_save_throw(VS_MAGIC))
		    {
			player_lose_health(roll(6, 6));
			if (player_get_health() <= 0)
			{
			    if (start == player_get_pos())
				death('b');
			    else
				death(moat(start->y, start->x)->t_type);
			}
			used = true;
			if (terse)
			    msg("the %s hits", name);
			else
			    msg("you are hit by the %s", name);
		    }
		    else
			msg("the %s whizzes by you", name);
		}
		mvaddcch(pos.y, pos.x, dirch);
		refresh();
	}
    }
    for (c2 = spotpos; c2 < c1; c2++)
	mvaddcch(c2->y, c2->x, chat(c2->y, c2->x));
}

/*
 * charge_str:
 *	Return an appropriate string for a wand charge
 */
char *
charge_str(THING *obj)
{
    static char buf[20];

    if (!(obj->o_flags & ISKNOW))
	buf[0] = '\0';
    else if (terse)
	sprintf(buf, " [%d]", obj->o_charges);
    else
	sprintf(buf, " [%d charges]", obj->o_charges);
    return buf;
}
