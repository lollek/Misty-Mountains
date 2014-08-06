/*
 * All the fighting gets done here
 *
 * @(#)fight.c	4.67 (Berkeley) 09/06/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "rogue.h"
#include "status_effects.h"
#include "command.h"
#include "io.h"
#include "armor.h"
#include "chase.h"

#define	EQSTR(a, b)	(strcmp(a, b) == 0)

static const char *prname(const char *mname, bool upper);
static void thunk(THING *weap, const char *mname, bool noend);
static bool roll_em(THING *thatt, THING *thdef, THING *weap, bool hurl);

char *h_names[] = {		/* strings for hitting */
	" scored an excellent hit on ",
	" hit ",
	" have injured ",
	" swing and hit ",
	" scored an excellent hit on ",
	" hit ",
	" has injured ",
	" swings and hits "
};

char *m_names[] = {		/* strings for missing */
	" miss",
	" swing and miss",
	" barely miss",
	" don't hit",
	" misses",
	" swings and misses",
	" barely misses",
	" doesn't hit",
};

/*
 * adjustments to hit probabilities due to strength
 */
static int str_plus[] = {
    -7, -6, -5, -4, -3, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3,
};

/*
 * adjustments to damage done due to strength
 */
static int add_dam[] = {
    -7, -6, -5, -4, -3, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3,
    3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6
};

/*
 * fight:
 *	The player attacks the monster.
 */
int
fight(coord *mp, THING *weap, bool thrown)
{
    THING *tp = moat(mp->y, mp->x);
    bool did_hit = true;
    char *mname, ch;

    /* Find the monster we want to fight */
    if (wizard && tp == NULL)
	msg("Fight what @ %d,%d", mp->y, mp->x);

    /* Since we are fighting, things are not quiet so no healing takes place */
    stop_counting(false);
    quiet = 0;
    runto(mp);

    /* Let him know it was really a xeroc (if it was one) */
    ch = '\0';
    if (tp->t_type == 'X' && tp->t_disguise != 'X' && !is_blind(&player))
    {
	tp->t_disguise = 'X';
	if (is_hallucinating(&player)) {
	    ch = (char)(rnd(26) + 'A');
	    mvaddcch(tp->t_pos.y, tp->t_pos.x, ch);
	}
	msg(is_hallucinating(&player)
	    ? "heavy!  That's a nasty critter!"
	    : "wait!  That's a xeroc!");
	if (!thrown)
	    return false;
    }
    mname = set_mname(tp);
    did_hit = false;
    has_hit = (terse && !to_death);
    if (roll_em(&player, tp, weap, thrown))
    {
	did_hit = false;
	if (thrown)
	    thunk(weap, mname, terse);
	else
	    hit((char *) NULL, mname, terse);
	if (is_confusing(&player))
	{
	    did_hit = true;
	    set_confused(tp, true);
	    set_confusing(&player, false);
	    endmsg();
	    has_hit = false;
	    msg("your hands stop glowing %s", pick_color("red"));
	}
	if (tp->t_stats.s_hpt <= 0)
	    killed(tp, true);
	else if (did_hit && !is_blind(&player))
	    msg("%s appears confused", mname);
	did_hit = true;
    }
    else
	if (thrown)
	    bounce(weap, mname, terse);
	else
	    miss((char *) NULL, mname, terse);
    return did_hit;
}

/** attack:
 * The monster attacks the player */
int
attack(THING *mp)
{
    char *mname;
    int oldhp;

    /* Since this is an attack, stop running and any healing that was
     * going on at the time */
    stop_counting(false);
    quiet = 0;
    if (to_death && !on(*mp, ISTARGET))
    {
	to_death = false;
	kamikaze = false;
    }
    if (mp->t_type == 'X' && mp->t_disguise != 'X' && !is_blind(&player))
    {
	mp->t_disguise = 'X';
	if (is_hallucinating(&player))
	    mvaddcch(mp->t_pos.y, mp->t_pos.x, rnd(26) + 'A');
    }
    mname = set_mname(mp);
    oldhp = pstats.s_hpt;
    if (roll_em(mp, &player, (THING *) NULL, false))
    {
	if (mp->t_type != 'I')
	{
	    if (has_hit)
		addmsg(".  ");
	    hit(mname, (char *) NULL, false);
	}
	else
	    if (has_hit)
		endmsg();
	has_hit = false;
	if (pstats.s_hpt <= 0)
	    death(mp->t_type);	/* Bye bye life ... */
	else if (!kamikaze)
	{
	    oldhp -= pstats.s_hpt;
	    if (oldhp > max_hit)
		max_hit = oldhp;
	    if (pstats.s_hpt <= max_hit)
		to_death = false;
	}
	if (!is_cancelled(mp))
	    switch (mp->t_type)
	    {
		case 'A':
		    /*
		     * If an aquator hits, you can lose armor class.
		     */
		    rust_armor(cur_armor);
		when 'I':
		    /*
		     * The ice monster freezes you
		     */
		    player.t_flags &= ~ISRUN;
		    if (!no_command)
		    {
			addmsg("you are frozen");
			if (!terse)
			    addmsg(" by the %s", mname);
			endmsg();
		    }
		    no_command += rnd(2) + 2;
		    if (no_command > 50)
			death('h');
		when 'R':
		    /*
		     * Rattlesnakes have poisonous bites
		     */
		    if (!save(VS_POISON))
		    {
			if (!ISWEARING(R_SUSTSTR))
			{
			    chg_str(-1);
			    if (!terse)
				msg("you feel a bite in your leg and now feel weaker");
			    else
				msg("a bite has weakened you");
			}
			else if (!to_death)
			{
			    if (!terse)
				msg("a bite momentarily weakens you");
			    else
				msg("bite has no effect");
			}
		    }
		when 'W':
		case 'V':
		    /*
		     * Wraiths might drain energy levels, and Vampires
		     * can steal max_hp
		     */
		    if (rnd(100) < (mp->t_type == 'W' ? 15 : 30))
		    {
			int fewer;

			if (mp->t_type == 'W')
			{
			    if (pstats.s_exp == 0)
				death('W');		/* All levels gone */
			    if (--pstats.s_lvl == 0)
			    {
				pstats.s_exp = 0;
				pstats.s_lvl = 1;
			    }
			    else
				pstats.s_exp = e_levels[pstats.s_lvl-1]+1;
			    fewer = roll(1, 10);
			}
			else
			    fewer = roll(1, 3);
			pstats.s_hpt -= fewer;
			max_hp -= fewer;
			if (pstats.s_hpt <= 0)
			    pstats.s_hpt = 1;
			if (max_hp <= 0)
			    death(mp->t_type);
			msg("you suddenly feel weaker");
		    }
		when 'F':
		    /*
		     * Venus Flytrap stops the poor guy from moving
		     */
		    player.t_flags |= ISHELD;
		    ++vf_hit;
		    if (--pstats.s_hpt <= 0)
			death('F');
		when 'L':
		{
		    /*
		     * Leperachaun steals some gold
		     */
		    int lastpurse;

		    lastpurse = purse;
		    purse -= GOLDCALC;
		    if (!save(VS_MAGIC))
			purse -= GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
		    if (purse < 0)
			purse = 0;
		    remove_mon(&mp->t_pos, mp, false);
                    mp=NULL;
		    if (purse != lastpurse)
			msg("your purse feels lighter");
		}
		when 'N':
		{
		    THING *obj, *steal;
		    int nobj;

		    /*
		     * Nymph's steal a magic item, look through the pack
		     * and pick out one we like.
		     */
		    steal = NULL;
		    for (nobj = 0, obj = pack; obj != NULL; obj = obj->l_next)
			if (obj != cur_armor && obj != cur_weapon
			    && obj != cur_ring[LEFT] && obj != cur_ring[RIGHT]
			    && is_magic(obj) && rnd(++nobj) == 0)
				steal = obj;
		    if (steal != NULL)
		    {
			remove_mon(&mp->t_pos, moat(mp->t_pos.y, mp->t_pos.x), false);
                        mp=NULL;
			leave_pack(steal, false, false);
			msg("she stole %s!", inv_name(steal, true, true));
			discard(steal);
		    }
		}
		otherwise:
		    break;
	    }
    }
    else if (mp->t_type != 'I')
    {
	if (has_hit)
	{
	    addmsg(".  ");
	    has_hit = false;
	}
	if (mp->t_type == 'F')
	{
	    pstats.s_hpt -= vf_hit;
	    if (pstats.s_hpt <= 0)
		death(mp->t_type);	/* Bye bye life ... */
	}
	miss(mname, (char *) NULL, false);
    }
    if (fight_flush && !to_death)
	flushinp();
    stop_counting(false);
    status();
    return(mp == NULL ? -1 : 0);
}

/*
 * set_mname:
 *	return the monster name for the given monster
 */
char *
set_mname(THING *tp)
{
    int ch;
    char *mname;
    static char tbuf[MAXSTR] = { 't', 'h', 'e', ' ' };

    if (!see_monst(tp) && !on(player, SEEMONST))
	return (terse ? "it" : "something");
    else if (is_hallucinating(&player))
    {
	move(tp->t_pos.y, tp->t_pos.x);
	ch = incch();
	if (!isupper(ch))
	    ch = rnd(26);
	else
	    ch -= 'A';
	mname = monsters[ch].m_name;
    }
    else
	mname = monsters[tp->t_type - 'A'].m_name;
    strcpy(&tbuf[4], mname);
    return tbuf;
}

/** swing:
 * Returns true if the swing hits */
int
swing(int at_lvl, int op_arm, int wplus)
{
    return at_lvl + wplus + rnd(20) >= op_arm;
}

/** roll_em:
 * Roll several attacks */
static bool
roll_em(THING *thatt, THING *thdef, THING *weap, bool hurl)
{
    struct stats *att = &thatt->t_stats;
    struct stats *def = &thdef->t_stats;
    const char *cp = weap == NULL ? att->s_dmg : weap->o_damage;
    int hplus = weap == NULL ? 0 : weap->o_hplus;
    int dplus = weap == NULL ? 0 : weap->o_dplus;
    int def_arm = get_ac(thdef);
    bool is_player = thatt == &player;
    bool did_hit = false;
    char vf_damage[13];

    /* Venus Flytraps have a different kind of dmg system */
    if (thatt->o_type == 'F')
    {
      sprintf(vf_damage, "%dx1", vf_hit);
      cp = vf_damage;
    }

    if (is_player)
    {
	if (ISRING(LEFT, R_ADDDAM))
	    dplus += cur_ring[LEFT]->o_arm;
	else if (ISRING(LEFT, R_ADDHIT))
	    hplus += cur_ring[LEFT]->o_arm;

	if (ISRING(RIGHT, R_ADDDAM))
	    dplus += cur_ring[RIGHT]->o_arm;
	else if (ISRING(RIGHT, R_ADDHIT))
	    hplus += cur_ring[RIGHT]->o_arm;

	if (hurl)
	{
	    if ((weap->o_flags & ISMISL) && cur_weapon != NULL &&
	      cur_weapon->o_which == weap->o_launch)
	    {
		cp = weap->o_hurldmg;
		hplus += cur_weapon->o_hplus;
		dplus += cur_weapon->o_dplus;
	    }
	    else if (weap->o_launch < 0)
		cp = weap->o_hurldmg;
	}
    }
    /* If the creature being attacked is not running (alseep or held)
     * then the attacker gets a plus four bonus to hit. */
    if (!on(*thdef, ISRUN))
	hplus += 4;
    while(cp != NULL && *cp != '\0')
    {
	int ndice;
	int nsides;
	if (sscanf(cp, "%dx%d", &ndice, &nsides) == EOF)
	    break;
	if (swing(att->s_lvl, def_arm, hplus + str_plus[att->s_str]))
	{
	    int proll = roll(ndice, nsides);
	    int damage = dplus + proll + add_dam[att->s_str];
	
	    if (wizard && ndice + nsides > 0 && proll <= 0)
		msg("Damage for %dx%d came out %d, dplus = %d, add_dam = %d, def_arm = %d", ndice, nsides, proll, dplus, add_dam[att->s_str], def_arm);
	    def->s_hpt -= max(0, damage);
	    did_hit = true;
	}
	if ((cp = strchr(cp, '/')) == NULL)
	    break;
	cp++;
    }
    return did_hit;
}

/*
 * prname:
 *	The print name of a combatant
 */
static const char *
prname(const char *mname, bool upper)
{
    static char tbuf[MAXSTR];

    *tbuf = '\0';
    if (mname == 0)
	strcpy(tbuf, "you"); 
    else
	strcpy(tbuf, mname);
    if (upper)
	*tbuf = (char) toupper(*tbuf);
    return tbuf;
}

/*
 * thunk:
 *	A missile hits a monster
 */
static void
thunk(THING *weap, const char *mname, bool noend)
{
    if (to_death)
	return;
    if (weap->o_type == WEAPON)
	addmsg("the %s hits ", weap_info[weap->o_which].oi_name);
    else
	addmsg("you hit ");
    addmsg("%s", mname);
    if (!noend)
	endmsg();
}

/*
 * hit:
 *	Print a message to indicate a succesful hit
 */

void
hit(const char *er, const char *ee, bool noend)
{
    int i;
    char *s;
    extern char *h_names[];

    if (to_death)
	return;
    addmsg(prname(er, true));
    if (terse)
	s = " hit";
    else
    {
	i = rnd(4);
	if (er != NULL)
	    i += 4;
	s = h_names[i];
    }
    addmsg(s);
    if (!terse)
	addmsg(prname(ee, false));
    if (!noend)
	endmsg();
}

/*
 * miss:
 *	Print a message to indicate a poor swing
 */
void
miss(const char *er, const char *ee, bool noend)
{
    int i;
    extern char *m_names[];

    if (to_death)
	return;
    addmsg(prname(er, true));
    if (terse)
	i = 0;
    else
	i = rnd(4);
    if (er != NULL)
	i += 4;
    addmsg(m_names[i]);
    if (!terse)
	addmsg(" %s", prname(ee, false));
    if (!noend)
	endmsg();
}

/*
 * bounce:
 *	A missile misses a monster
 */
void
bounce(THING *weap, const char *mname, bool noend)
{
    if (to_death)
	return;
    if (weap->o_type == WEAPON)
	addmsg("the %s misses ", weap_info[weap->o_which].oi_name);
    else
	addmsg("you missed ");
    addmsg(mname);
    if (!noend)
	endmsg();
}

/*
 * remove_mon:
 *	Remove a monster from the screen
 */
void
remove_mon(coord *mp, THING *tp, bool waskill)
{
    THING *obj, *nexti;

    for (obj = tp->t_pack; obj != NULL; obj = nexti)
    {
	nexti = obj->l_next;
	obj->o_pos = tp->t_pos;
	detach(tp->t_pack, obj);
	if (waskill)
	    fall(obj, false);
	else
	    discard(obj);
    }
    moat(mp->y, mp->x) = NULL;
    mvaddcch(mp->y, mp->x, tp->t_oldch);
    detach(mlist, tp);
    if (on(*tp, ISTARGET))
    {
	kamikaze = false;
	to_death = false;
	if (fight_flush)
	    flushinp();
    }
    discard(tp);
}

/*
 * killed:
 *	Called to put a monster to death
 */
void
killed(THING *tp, bool pr)
{
    char *mname;

    if (game_type == DEFAULT)
      pstats.s_exp += tp->t_stats.s_exp;

    /*
     * If the monster was a venus flytrap, un-hold him
     */
    switch (tp->t_type)
    {
	case 'F':
	    player.t_flags &= ~ISHELD;
	    vf_hit = 0;
	when 'L':
	{
	    THING *gold;

	    if (fallpos(&tp->t_pos, &tp->t_room->r_gold) && level >= max_level)
	    {
		gold = new_item();
		gold->o_type = GOLD;
		gold->o_goldval = GOLDCALC;
		if (save(VS_MAGIC))
		    gold->o_goldval += GOLDCALC + GOLDCALC
				     + GOLDCALC + GOLDCALC;
		attach(tp->t_pack, gold);
	    }
	}
    }
    /*
     * Get rid of the monster.
     */
    mname = set_mname(tp);
    remove_mon(&tp->t_pos, tp, true);
    if (pr)
    {
	if (has_hit)
	{
	    addmsg(".  Defeated ");
	    has_hit = false;
	}
	else
	{
	    if (!terse)
		addmsg("you have ");
	    addmsg("defeated ");
	}
	msg(mname);
    }
    /*
     * Do adjustments if he went up a level
     */
    check_level();
    if (fight_flush)
	flushinp();
}
