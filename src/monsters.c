/*
 * File with various monster functions in it
 *
 * @(#)monsters.c	4.46 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "rogue.h"
#include "status_effects.h"
#include "io.h"
#include "pack.h"
#include "scrolls.h"

/*
 * List of monsters in rough order of vorpalness
 */
static char lvl_mons[] =  {
    'K', 'E', 'B', 'S', 'H', 'I', 'R', 'O', 'Z', 'L', 'C', 'Q', 'A',
    'N', 'Y', 'F', 'T', 'W', 'P', 'X', 'U', 'M', 'V', 'G', 'J', 'D'
};

static char wand_mons[] = {
    'K', 'E', 'B', 'S', 'H',   0, 'R', 'O', 'Z',   0, 'C', 'Q', 'A',
      0, 'Y',   0, 'T', 'W', 'P',   0, 'U', 'M', 'V', 'G', 'J',   0
};

/*
 * randmonster:
 *	Pick a monster to show up.  The lower the level,
 *	the meaner the monster.
 */
char
randmonster(bool wander)
{
    const char *mons = (wander ? wand_mons : lvl_mons);

    while (true)
    {
	int d = level + (rnd(10) - 6);
	if (d < 0)
	    d = rnd(5);
	if (d > 25)
	    d = rnd(5) + 21;
	if (mons[d] != 0)
	    return mons[d];
    }
}

/*
 * new_monster:
 *	Pick a new monster and add it to the list
 */

void
new_monster(THING *tp, char type, coord *cp)
{
    struct monster *mp;
    int lev_add;

    if ((lev_add = level - AMULETLEVEL) < 0)
	lev_add = 0;
    attach(mlist, tp);
    tp->t_type = type;
    tp->t_disguise = type;
    tp->t_pos = *cp;
    move(cp->y, cp->x);
    tp->t_oldch = incch();
    tp->t_room = roomin(cp);
    moat(cp->y, cp->x) = tp;
    mp = &monsters[tp->t_type-'A'];
    tp->t_stats.s_lvl = mp->m_stats.s_lvl + lev_add;
    tp->t_stats.s_maxhp = tp->t_stats.s_hpt = roll(tp->t_stats.s_lvl, 8);
    tp->t_stats.s_arm = mp->m_stats.s_arm - lev_add;
    strcpy(tp->t_stats.s_dmg,mp->m_stats.s_dmg);
    tp->t_stats.s_str = mp->m_stats.s_str;
    tp->t_stats.s_exp = mp->m_stats.s_exp + lev_add * 10 + exp_add(tp);
    tp->t_flags = mp->m_flags;
    if (level > 29)
	tp->t_flags |= ISHASTE;
    tp->t_turn = true;
    tp->t_pack = NULL;
    if (player_has_ring_with_ability(R_AGGR))
	runto(cp);
    if (type == 'X')
	tp->t_disguise = rnd_thing();
}

/*
 * expadd:
 *	Experience to add for this monster's level/hit points
 */
int
exp_add(THING *tp)
{
    int mod;

    if (tp->t_stats.s_lvl == 1)
	mod = tp->t_stats.s_maxhp / 8;
    else
	mod = tp->t_stats.s_maxhp / 6;
    if (tp->t_stats.s_lvl > 9)
	mod *= 20;
    else if (tp->t_stats.s_lvl > 6)
	mod *= 4;
    return mod;
}

/*
 * wanderer:
 *	Create a new wandering monster and aim it at the player
 */

void
wanderer(void)
{
    THING *tp;
    static coord cp;

    tp = new_item();
    do
    {
	find_floor((struct room *) NULL, &cp, false, true);
    } while (roomin(&cp) == proom);
    new_monster(tp, randmonster(true), &cp);
    if (on(player, SEEMONST))
    {
	if (is_hallucinating(&player))
	    addcch((rnd(26) + 'A') | A_STANDOUT);
	else
	    addcch(tp->t_type | A_STANDOUT);
    }
    runto(&tp->t_pos);
}

/*
 * wake_monster:
 *	What to do when the hero steps next to a monster
 */
THING *
wake_monster(int y, int x)
{
    THING *tp = moat(y, x);
    struct room *rp;
    char ch, *mname;

    assert_or_die(tp != NULL, "can't find monster in wake_monster");
    ch = tp->t_type;
    /*
     * Every time he sees mean monster, it might start chasing him
     */
    if (!on(*tp, ISRUN) && rnd(3) != 0 && on(*tp, ISMEAN) && !on(*tp, ISHELD)
	&& !player_has_ring_with_ability(R_STEALTH) && !is_levitating(&player))
    {
	tp->t_dest = &hero;
	tp->t_flags |= ISRUN;
    }
    if (ch == 'M' && !is_blind(&player) && !is_hallucinating(&player)
	&& !is_found(tp) && !is_cancelled(tp) && on(*tp, ISRUN))
    {
        rp = proom;
	if ((rp != NULL && !(rp->r_flags & ISDARK))
	    || dist(y, x, hero.y, hero.x) < LAMPDIST)
	{
	    set_found(tp, true);
	    if (!save(VS_MAGIC))
	    {
		mname = set_mname(tp);
		addmsg("%s", mname);
		if (strcmp(mname, "it") != 0)
		    addmsg("'");
		msg("s gaze has confused you. ");
		become_confused(false);
	    }
	}
    }
    /*
     * Let greedy ones guard gold
     */
    if (on(*tp, ISGREED) && !on(*tp, ISRUN))
    {
	tp->t_flags |= ISRUN;
	if (proom->r_goldval)
	    tp->t_dest = &proom->r_gold;
	else
	    tp->t_dest = &hero;
    }
    return tp;
}

/*
 * give_pack:
 *	Give a pack to a monster if it deserves one
 */

void
give_pack(THING *tp)
{
    if (level >= max_level && rnd(100) < monsters[tp->t_type-'A'].m_carry)
	attach(tp->t_pack, new_thing());
}

/*
 * save_throw:
 *	See if a creature save against something
 */
int
save_throw(int which, THING *tp)
{
    int need;

    need = 14 + which - tp->t_stats.s_lvl / 2;
    return (roll(1, 20) >= need);
}

/** save:
 * See if he saves against various nasty things */
int
save(int which)
{
  if (which == VS_MAGIC)
  {
    int i;
    for (i = 0; i < RING_SLOTS_SIZE; ++i)
    {
      THING *ring = equipped_item(ring_slots[i]);
      if (ring != NULL && ring->o_which == R_PROTECT)
        which -= ring->o_arm;
    }
  }
  return save_throw(which, &player);
}

void
runto(coord *runner)
{
  THING *tp = moat(runner->y, runner->x);

  /* If we couldn't find him, something is funny */
  assert (tp != NULL);
  /* msg("couldn't find monster in runto at (%d,%d)", runner->y, runner->x); */

  /* Start the beastie running */
  tp->t_flags |= ISRUN;
  tp->t_flags &= ~ISHELD;
  tp->t_dest = find_dest(tp);
}

coord *
find_dest(THING *tp)
{
    THING *obj;
    int prob;

    if ((prob = monsters[tp->t_type - 'A'].m_carry) <= 0 || tp->t_room == proom
	|| see_monst(tp))
	    return &hero;
    for (obj = lvl_obj; obj != NULL; obj = obj->l_next)
    {
	if (obj->o_type == SCROLL && obj->o_which == S_SCARE)
	    continue;
	if (roomin(&obj->o_pos) == tp->t_room && rnd(100) < prob)
	{
	    for (tp = mlist; tp != NULL; tp = tp->l_next)
		if (tp->t_dest == &obj->o_pos)
		    break;
	    if (tp == NULL)
		return &obj->o_pos;
	}
    }
    return &hero;
}

