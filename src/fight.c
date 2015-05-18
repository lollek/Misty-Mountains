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
#include <assert.h>

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
#include "player.h"
#include "weapons.h"
#include "things.h"
#include "rip.h"
#include "rogue.h"

#include "fight.h"

struct attack_modifier
{
  int to_hit;
  int to_dmg;
  char* damage;
};

/** add_player_attack_modifiers
 * Add item bonuses to damage and to-hit */
static void
add_ring_attack_modifiers(struct attack_modifier* mod)
{
  for (int i = 0; i < RING_SLOTS_SIZE; ++i)
  {
    THING* ring = pack_equipped_item(ring_slots[i]);
    if (ring == NULL)
      continue;

    else if (ring->o_which == R_ADDDAM)
      mod->to_dmg += ring->o_arm;
    else if (ring->o_which == R_ADDHIT)
      mod->to_dmg += ring->o_arm;
  }
}

static void
add_strength_attack_modifiers(int strength, struct attack_modifier* mod)
{
  switch (strength)
  {
    case  0: mod->to_hit -= 7; mod->to_dmg -= 7; return;
    case  1: mod->to_hit -= 6; mod->to_dmg -= 6; return;
    case  2: mod->to_hit -= 5; mod->to_dmg -= 5; return;
    case  3: mod->to_hit -= 4; mod->to_dmg -= 4; return;
    case  4: mod->to_hit -= 3; mod->to_dmg -= 3; return;
    case  5: mod->to_hit -= 2; mod->to_dmg -= 2; return;
    case  6: mod->to_hit -= 1; mod->to_dmg -= 1; return;

    default: return;

    case 16:                   mod->to_dmg += 1; return;
    case 17: mod->to_hit += 1; mod->to_dmg += 1; return;
    case 18: mod->to_hit += 1; mod->to_dmg += 2; return;
    case 19: mod->to_hit += 1; mod->to_dmg += 3; return;
    case 20: mod->to_hit += 1; mod->to_dmg += 3; return;
    case 21: mod->to_hit += 2; mod->to_dmg += 4; return;
    case 22: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 23: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 24: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 25: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 26: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 27: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 28: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 29: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 30: mod->to_hit += 2; mod->to_dmg += 5; return;
    case 31: mod->to_hit += 3; mod->to_dmg += 6; return;
  }
}

static void
calculate_attacker(THING* attacker, THING* weapon, bool thrown,
                  struct attack_modifier* mod)
{
  if (weapon != NULL)
  {
    mod->damage  = weapon->o_damage;
    mod->to_hit += weapon->o_hplus;
    mod->to_dmg += weapon->o_dplus;
  }

  add_strength_attack_modifiers(attacker->t_stats.s_str, mod);

  /* Player stuff */
  if (is_player(attacker))
  {
    add_ring_attack_modifiers(mod);
    if (thrown)
    {
      THING const* held_weapon = pack_equipped_item(EQUIPMENT_RHAND);
      if ((weapon->o_flags & ISMISL) && held_weapon != NULL
          && held_weapon->o_which == weapon->o_launch)
      {
        mod->damage  = weapon->o_hurldmg;
        mod->to_hit += held_weapon->o_hplus;
        mod->to_dmg += held_weapon->o_dplus;
      }
      else if (weapon->o_launch == -1)
        mod->damage = weapon->o_hurldmg;
    }
  }

  /* Venus Flytraps have a different kind of dmg system */
  else if (attacker->o_type == 'F')
  {
    static char f_damage[13];
    sprintf(f_damage, "%dx1", vf_hit);
    mod->damage = f_damage;
  }
}

static bool
roll_attacks(THING* attacker, THING* defender, THING* weapon, bool thrown)
{
  struct attack_modifier mod = { 0, 0, attacker->t_stats.s_dmg };
  calculate_attacker(attacker, weapon, thrown, &mod);

  /* If attacked creature is not running (asleep or held)
   * the attacker gets a bonus to hit */
  if (!on(*defender, ISRUN))
    mod.to_hit += 4;

  assert(mod.damage != NULL);
  bool did_hit = false;

  while (*mod.damage != '\0')
  {
    int defense = armor_for_thing(defender);
    int dices;
    int dice_sides;
    if (sscanf(mod.damage, "%dx%d", &dices, &dice_sides) == EOF)
      break;

    if (fight_swing_hits(attacker->t_stats.s_lvl, defense, mod.to_hit))
    {
      defender->t_stats.s_hpt -= max(0, roll(dices, dice_sides) + mod.to_dmg);
      did_hit = true;
    }

    mod.damage = strchr(mod.damage, '/');
    if (mod.damage == NULL)
      break;
    ++mod.damage;
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
    if (tp->t_type == 'X' && tp->t_disguise != 'X' && !player_is_blind())
    {
	tp->t_disguise = 'X';
	if (player_is_hallucinating()) {
	    ch = (char)(rnd(26) + 'A');
	    mvaddcch(tp->t_pos.y, tp->t_pos.x, ch);
	}
	msg(player_is_hallucinating()
	    ? "heavy!  That's a nasty critter!"
	    : "wait!  That's a xeroc!");
	if (!thrown)
	    return false;
    }
    mname = set_mname(tp);
    did_hit = false;
    has_hit = (terse && !to_death);
    /* TODO: remove __player_ptr reference */
    if (roll_attacks(__player_ptr(), tp, weap, thrown)) 
    {
	did_hit = false;
	if (thrown)
	    thunk(weap, mname, terse);
	else
	    print_attack(true, (char *) NULL, mname, terse);
	if (player_has_confusing_attack())
	{
	    did_hit = true;
	    monster_set_confused(tp);
            player_remove_confusing_attack();
	    endmsg();
	    has_hit = false;
	    msg("your hands stop glowing %s", pick_color("red"));
	}
	if (tp->t_stats.s_hpt <= 0)
	    monster_on_death(tp, true);
	else if (did_hit && !player_is_blind())
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
    if (mp->t_type == 'X' && mp->t_disguise != 'X' && !player_is_blind())
    {
	mp->t_disguise = 'X';
	if (player_is_hallucinating())
	    mvaddcch(mp->t_pos.y, mp->t_pos.x, rnd(26) + 'A');
    }
    mname = set_mname(mp);
    oldhp = player_get_health();
    /* TODO: Remove __player_ptr() reference */
    if (roll_attacks(mp, __player_ptr(), (THING *) NULL, false))
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
	if (player_get_health() <= 0)
	    death(mp->t_type);	/* Bye bye life ... */
	else if (!kamikaze)
	{
	    oldhp -= player_get_health();
	    if (oldhp > max_hit)
		max_hit = oldhp;
	    if (player_get_health() <= max_hit)
		to_death = false;
	}
	if (!monster_is_cancelled(mp))
	    switch (mp->t_type)
	    {
		case 'A':
		    /* If an aquator hits, you can lose armor class */
		    armor_rust();
                    break;
		case 'I':
		    /* The ice monster freezes you */
		    player_stop_running();
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
                    break;
		case 'R':
		    /* Rattlesnakes have poisonous bites */
		    if (!player_save_throw(VS_POISON))
		    {
			if (!player_has_ring_with_ability(R_SUSTSTR))
			{
			    player_modify_strength(-1);
			    msg(terse
			      ? "a bite has weakened you"
			      : "you feel a bite in your leg and now feel weaker");
			}
			else if (!to_death)
			    msg(terse
				? "bite has no effect"
				: "a bite momentarily weakens you");
		    }
                    break;
		case 'W':
		case 'V':
		    /* Wraiths might drain energy levels, and Vampires
		     * can steal max_hp */
		    if (rnd(100) < (mp->t_type == 'W' ? 15 : 30))
		    {
			int fewer;

			if (mp->t_type == 'W')
			{
			    if (player_get_exp() == 0)
				death('W');		/* All levels gone */
                            player_lower_level();
			    fewer = roll(1, 10);
			}
			else
			    fewer = roll(1, 3);
                        player_lose_health(fewer);
			player_modify_max_health(-fewer);
			while (player_get_health() <= 0)
                            player_restore_health(1, false);
			if (player_get_max_health() <= 0)
			    death(mp->t_type);
			msg("you suddenly feel weaker");
		    }
                    break;
		case 'F':
		    /* Venus Flytrap stops the poor guy from moving */
		    player_set_held();
		    ++vf_hit;
                    player_lose_health(1);
		    if (player_get_health() <= 0)
			death('F');
                    break;
		case 'L':
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
                break;
		case 'N':
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
			_discard(&steal);
		    }
		}
                break;
		default:
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
	    player_lose_health(vf_hit);
	    if (player_get_health() <= 0)
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
