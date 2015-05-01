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

#include "status_effects.h"
#include "io.h"
#include "pack.h"
#include "scrolls.h"
#include "list.h"
#include "rings.h"
#include "rooms.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "rogue.h"

#include "monster.h"

struct monster monsters[26] =
    {
/* Name		 CARRY	FLAG    str, exp, lvl, amr, hpt, dmg,      ,maxhp */
{ "aquator",	   0,	ISMEAN,	{ 10, 20,   5,   2,  1, "0x0/0x0", 0 } },
{ "bat",	   0,	ISFLY,	{ 10,  1,   1,   3,  1, "1x2", 0 } },
{ "centaur",	  15,	0,	{ 10, 17,   4,   4,  1, "1x2/1x5/1x5", 0 } },
{ "dragon",	 100,	ISMEAN,	{ 10,5000, 10,  -1,  1, "1x8/1x8/3x10", 0 } },
{ "emu",	   0,	ISMEAN,	{ 10,  2,   1,   7,  1, "1x2", 0 } },
{ "venus flytrap", 0,	ISMEAN,	{ 10, 80,   8,   3,  1, "000x0", 0 } },
{ "griffin",	  20,	ISMEAN|ISFLY|ISREGEN, { 10,2000, 13,   2, 1, "4x3/3x5", 0 } },
{ "hobgoblin",	   0,	ISMEAN,	{ 10,  3,   1,   5,  1, "1x8", 0 } },
{ "ice monster",   0,	0,	{ 10,  5,   1,   9,  1, "0x0", 0 } },
{ "jabberwock",   70,	0,	{ 10,3000, 15,   6,  1, "2x12/2x4", 0 } },
{ "kestrel",	   0,	ISMEAN|ISFLY,	{ 10,  1,    1,   7, 1, "1x4", 0 } },
{ "leprechaun",	   0,	0,	{ 10, 10,   3,   8,  1, "1x1", 0 } },
{ "medusa",	  40,	ISMEAN,	{ 10,200,   8,   2,  1, "3x4/3x4/2x5", 0 } },
{ "nymph",	 100,	0,	{ 10, 37,   3,   9,  1, "0x0", 0 } },
{ "orc",	  15,	ISGREED,{ 10,  5,   1,   6,  1, "1x8", 0 } },
{ "phantom",	   0,	ISINVIS,{ 10,120,   8,   3,  1, "4x4", 0 } },
{ "quagga",	   0,	ISMEAN,	{ 10, 15,   3,   3,  1, "1x5/1x5", 0 } },
{ "rattlesnake",   0,	ISMEAN,	{ 10,  9,   2,   3,  1, "1x6", 0 } },
{ "snake",	   0,	ISMEAN,	{ 10,  2,   1,   5,  1, "1x3", 0 } },
{ "troll",	  50,	ISREGEN|ISMEAN,{ 10, 120, 6, 4, 1, "1x8/1x8/2x6", 0 } },
{ "black unicorn", 0,	ISMEAN,	{ 10,190,   7,  -2, 1, "1x9/1x9/2x9", 0 } },
{ "vampire",	  20,	ISREGEN|ISMEAN,{ 10,350,   8,   1, 1, "1x10", 0 } },
{ "wraith",	   0,	0,	{ 10, 55,   5,   4,  1, "1x6", 0 } },
{ "xeroc",	  30,	0,	{ 10,100,   7,   7,  1, "4x4", 0 } },
{ "yeti",	  30,	0,	{ 10, 50,   4,   6,  1, "1x6/1x6", 0 } },
{ "zombie",	   0,	ISMEAN,	{ 10,  6,   2,   8,  1, "1x8", 0 } }
};

char
monster_random(bool wander)
{
  /* List of monsters in rough order of vorpalness */
  static const char lvl_mons[] =  {
    'K', 'E', 'B', 'S', 'H', 'I', 'R', 'O', 'Z', 'L', 'C', 'Q', 'A',
    'N', 'Y', 'F', 'T', 'W', 'P', 'X', 'U', 'M', 'V', 'G', 'J', 'D'
  };
  static const char wand_mons[] = {
    'K', 'E', 'B', 'S', 'H',  0 , 'R', 'O', 'Z',  0 , 'C', 'Q', 'A',
     0 , 'Y',  0 , 'T', 'W', 'P',  0 , 'U', 'M', 'V', 'G', 'J',  0
  };

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

/** monster_xp_worth
 * Experience to add for this monster's level/hit points */
static int
monster_xp_worth(THING *tp)
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

void
monster_new(THING *tp, char type, coord *cp)
{
  struct monster *mp;
  int lev_add = level - AMULETLEVEL;

  if (lev_add < 0)
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
  tp->t_stats.s_exp = mp->m_stats.s_exp + lev_add * 10 + monster_xp_worth(tp);
  tp->t_flags = mp->m_flags;

  if (level > 29)
    tp->t_flags |= ISHASTE;

  tp->t_turn = true;
  tp->t_pack = NULL;

  if (player_has_ring_with_ability(R_AGGR))
    monster_start_running(cp);

  if (type == 'X')
    tp->t_disguise = rnd_thing();
}

void
monster_new_random_wanderer(void)
{
  THING *tp = new_item();
  static coord cp;

  do
  {
    room_find_floor((struct room *) NULL, &cp, false, true);
  } while (roomin(&cp) == proom);

  monster_new(tp, monster_random(true), &cp);
  if (on(player, SEEMONST))
  {
    if (is_hallucinating(&player))
      addcch((rnd(26) + 'A') | A_STANDOUT);
    else
      addcch(tp->t_type | A_STANDOUT);
  }
  monster_start_running(&tp->t_pos);
}

THING *
monster_notice_player(int y, int x)
{
  coord *player_pos = player_get_pos();
  THING *tp = moat(y, x);
  char ch;

  assert_or_die(tp != NULL, "can't find monster in monster_notice_player");
  ch = tp->t_type;

  /* Every time he sees mean monster, it might start chasing him */
  if (!on(*tp, ISRUN) && rnd(3) != 0 && on(*tp, ISMEAN) && !on(*tp, ISHELD)
      && !player_has_ring_with_ability(R_STEALTH) && !is_levitating(&player))
  {
    tp->t_dest = player_pos;
    tp->t_flags |= ISRUN;
  }

  if (ch == 'M' && !is_blind(&player) && !is_hallucinating(&player)
      && !is_found(tp) && !is_cancelled(tp) && on(*tp, ISRUN))
  {
    struct room *rp = proom;
    if ((rp != NULL && !(rp->r_flags & ISDARK))
        || dist(y, x, player_pos->y, player_pos->x) < LAMPDIST)
    {
      set_found(tp, true);
      if (!player_save_throw(VS_MAGIC))
      {
        char *mname = set_mname(tp);
        addmsg("%s", mname);
        if (strcmp(mname, "it") != 0)
          addmsg("'");
        msg("s gaze has confused you. ");
        become_confused(false);
      }
    }
  }

  /* Let greedy ones guard gold */
  if (on(*tp, ISGREED) && !on(*tp, ISRUN))
  {
    tp->t_flags |= ISRUN;
    if (proom->r_goldval)
      tp->t_dest = &proom->r_gold;
    else
      tp->t_dest = player_pos;
  }
  return tp;
}

void
monster_give_pack(THING *tp)
{
  if (level >= max_level && rnd(100) < monsters[tp->t_type-'A'].m_carry)
    attach(tp->t_pack, new_thing());
}

int
monster_save_throw(int which, THING *tp)
{
  int need = 14 + which - tp->t_stats.s_lvl / 2;
  return (roll(1, 20) >= need);
}

void
monster_start_running(coord *runner)
{
  THING *tp = moat(runner->y, runner->x);
  assert_or_die (tp != NULL, "couldn't find monster in monster_start_running");

  /* Start the beastie running */
  tp->t_flags |= ISRUN;
  tp->t_flags &= ~ISHELD;
  tp->t_dest = monster_destination(tp);
}

coord *
monster_destination(THING *tp)
{
  THING *obj;
  int prob = monsters[tp->t_type - 'A'].m_carry;

  if (prob <= 0 || tp->t_room == proom || see_monst(tp))
    return player_get_pos();

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
  return player_get_pos();
}

void
monster_on_death(THING *tp, bool pr)
{
  char *mname;

  if (game_type == DEFAULT)
    player_earn_exp(tp->t_stats.s_exp);

  switch (tp->t_type)
  {
    /* If the monster was a venus flytrap, un-hold him */
    case 'F':
      player.t_flags &= ~ISHELD;
      vf_hit = 0;
      break;

    /* Leprechauns drop gold */
    case 'L':
      {
        THING *gold;

        if (fallpos(&tp->t_pos, &tp->t_room->r_gold) && level >= max_level)
        {
          gold = new_item();
          gold->o_type = GOLD;
          gold->o_goldval = GOLDCALC;
          if (player_save_throw(VS_MAGIC))
            gold->o_goldval += GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
          attach(tp->t_pack, gold);
        }
      }
  }

  /* Get rid of the monster. */
  mname = set_mname(tp);
  monster_remove_from_screen(&tp->t_pos, tp, true);
  if (pr)
  {
    if (has_hit)
    {
      addmsg(".  Defeated ");
      has_hit = false;
    }
    else
      msg("%s %s", terse
            ? "defeated"
            : "you have defeated",
          mname);
  }

  /* Do adjustments if he went up a level */
  player_check_for_level_up();
  if (fight_flush)
    flushinp();
}

void
monster_remove_from_screen(coord *mp, THING *tp, bool waskill)
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

