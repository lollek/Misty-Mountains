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

#include "status_effects.h"
#include "command.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "daemons.h"
#include "list.h"
#include "monster.h"
#include "rings.h"
#include "misc.h"
#include "level.h"
#include "rogue.h"

#include "fight.h"

/** add_player_attack_modifiers
 * Add item bonuses to damage and to-hit */
static void
add_player_attack_modifiers(int *damage, int *hit)
{
  int i;
  for (i = 0; i < RING_SLOTS_SIZE; ++i)
  {
    THING *ring = pack_equipped_item(ring_slots[i]);
    if (ring == NULL)
      continue;

    else if (ring->o_which == R_ADDDAM)
      *damage += ring->o_arm;
    else if (ring->o_which == R_ADDHIT)
      *hit += ring->o_arm;
  }
}

/** roll_em:
 * Roll several attacks */
static bool
roll_em(THING *thatt, THING *thdef, THING *weap, bool hurl)
{
  const int strbonus_to_hit[] = {
    -7, -6, -5, -4, -3, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3,
  };
  const int strbonus_to_dmg[] = {
    -7, -6, -5, -4, -3, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3,
    3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6
  };
  const struct stats *att = &thatt->t_stats;
  const char *cp = weap == NULL ? att->s_dmg : weap->o_damage;
  const bool attacker_is_player = thatt == &player;

  int hplus = weap == NULL ? 0 : weap->o_hplus;
  int dplus = weap == NULL ? 0 : weap->o_dplus;
  bool did_hit = false;
  char vf_damage[13];

  /* Venus Flytraps have a different kind of dmg system */
  if (thatt->o_type == 'F')
  {
    sprintf(vf_damage, "%dx1", vf_hit);
    cp = vf_damage;
  }

  if (attacker_is_player)
  {
    add_player_attack_modifiers(&dplus, &hplus);

    if (hurl)
    {
      THING *launcher = pack_equipped_item(EQUIPMENT_RHAND);
      if ((weap->o_flags & ISMISL) && launcher != NULL &&
          launcher->o_which == weap->o_launch)
      {
        cp = weap->o_hurldmg;
        hplus += launcher->o_hplus;
        dplus += launcher->o_dplus;
      }
      else if (weap->o_launch < 0)
        cp = weap->o_hurldmg;
    }
  }

  /* If the creature being attacked is not running (alseep or held)
   * then the attacker gets a plus four bonus to hit. */
  if (!on(*thdef, ISRUN))
    hplus += 4;

  while (cp != NULL && *cp != '\0')
  {
    struct stats *def = &thdef->t_stats;
    int def_arm = armor_for_thing(thdef);
    int ndice;
    int nsides;

    if (sscanf(cp, "%dx%d", &ndice, &nsides) == EOF)
      break;

    if (fight_swing_hits(att->s_lvl, def_arm, hplus + strbonus_to_hit[att->s_str]))
    {
      int proll = roll(ndice, nsides);
      int damage = dplus + proll + strbonus_to_dmg[att->s_str];

      if (wizard && ndice + nsides > 0 && proll <= 0)
        msg("Damage for %dx%d came out %d, "
            "dplus = %d, add_dam = %d, def_arm = %d",
            ndice, nsides, proll, dplus, strbonus_to_dmg[att->s_str], def_arm);

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
 * print_attack:
 *	Print a message to indicate a hit or miss
 */

static void
print_attack(bool hit, const char *att, const char *def, bool noend)
{
  const char *h_names[] = {
      " hit "
    , " scored an excellent hit on "
    , " have injured "
    , " swing and hit "
    , " hits "
    , " scored an excellent hit on "
    , " has injured "
    , " swings and hits "
  };
  const char *m_names[] = {
      " miss "
    , " swing and miss "
    , " barely miss "
    , " don't hit "
    , " misses "
    , " swings and misses "
    , " barely misses "
    , " doesn't hit "
  };
  int i;

  if (to_death)
    return;

  addmsg("%s", prname(att, true));

  i = terse ? 0 : rnd(4);
  if (att != NULL)
    i += 4;

  addmsg("%s", hit ? h_names[i] : m_names[i]);

  if (!terse)
    addmsg("%s", prname(def, false));

  if (!noend)
    endmsg();
}

int
fight_against_monster(coord *mp, THING *weap, bool thrown)
{
    THING *tp = moat(mp->y, mp->x);
    bool did_hit = true;
    char *mname, ch;

    /* Find the monster we want to fight */
    if (wizard && tp == NULL)
	msg("Fight what @ %d,%d", mp->y, mp->x);

    /* Since we are fighting, things are not quiet so no healing takes place */
    command_stop(false);
    daemon_reset_doctor();
    monster_start_running(mp);

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
	    print_attack(true, (char *) NULL, mname, terse);
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
	    monster_on_death(tp, true);
	else if (did_hit && !is_blind(&player))
	    msg("%s appears confused", mname);
	did_hit = true;
    }
    else
	if (thrown)
	    fight_missile_miss(weap, mname, terse);
	else
	    print_attack(false, (char *) NULL, mname, terse);
    return did_hit;
}

int
fight_against_player(THING *mp)
{
    char *mname;
    int oldhp;

    /* Since this is an attack, stop running and any healing that was
     * going on at the time */
    command_stop(false);
    daemon_reset_doctor();
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
	    print_attack(true, mname, (char *) NULL, false);
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
		    /* If an aquator hits, you can lose armor class */
		    armor_rust();
		when 'I':
		    /* The ice monster freezes you */
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
		    /* Rattlesnakes have poisonous bites */
		    if (!player_save_throw(VS_POISON))
		    {
			if (!player_has_ring_with_ability(R_SUSTSTR))
			{
			    chg_str(-1);
			    msg(terse
			      ? "a bite has weakened you"
			      : "you feel a bite in your leg and now feel weaker");
			}
			else if (!to_death)
			    msg(terse
				? "bite has no effect"
				: "a bite momentarily weakens you");
		    }
		when 'W':
		case 'V':
		    /* Wraiths might drain energy levels, and Vampires
		     * can steal max_hp */
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
		    /* Venus Flytrap stops the poor guy from moving */
		    player.t_flags |= ISHELD;
		    ++vf_hit;
		    if (--pstats.s_hpt <= 0)
			death('F');
		when 'L':
		{
		    /* Leperachaun steals some gold */
		    int lastpurse;

		    lastpurse = purse;
		    purse -= GOLDCALC;
		    if (!player_save_throw(VS_MAGIC))
			purse -= GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
		    if (purse < 0)
			purse = 0;
		    monster_remove_from_screen(&mp->t_pos, mp, false);
                    mp=NULL;
		    if (purse != lastpurse)
			msg("your purse feels lighter");
		}
		when 'N':
		{
		    /* Nymph's steal a magic item, look through the pack
		     * and pick out one we like. */
		    THING *steal = pack_find_magic_item();
		    if (steal != NULL)
		    {
			monster_remove_from_screen(&mp->t_pos, moat(mp->t_pos.y, mp->t_pos.x),
			           false);
			mp=NULL;
			pack_remove(steal, false, false);
			msg("she stole %s!", inv_name(steal, true));
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
	print_attack(false, mname, (char *) NULL, false);
    }
    if (fight_flush && !to_death)
	flushinp();
    command_stop(false);
    status();
    return(mp == NULL ? -1 : 0);
}

int
fight_swing_hits(int at_lvl, int op_arm, int wplus)
{
  return at_lvl + wplus + rnd(20) >= op_arm;
}

void
fight_missile_miss(THING *weap, const char *mname, bool noend)
{
  if (to_death)
    return;

  if (weap->o_type == WEAPON)
    addmsg("the %s misses ", weap_info[weap->o_which].oi_name);
  else
    addmsg("you missed ");

  addmsg("%s", mname);

  if (!noend)
    endmsg();
}
