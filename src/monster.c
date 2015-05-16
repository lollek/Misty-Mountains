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

#include "io.h"
#include "pack.h"
#include "scrolls.h"
#include "list.h"
#include "rings.h"
#include "rooms.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "weapons.h"
#include "things.h"
#include "os.h"
#include "state.h"
#include "rogue.h"

#include "monster.h"

THING *mlist = NULL;
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

bool
monsters_save_state(void)
{
  THING *ptr;
  int length;

  for (ptr = mlist, length = 0; ptr != NULL; ptr = ptr->l_next)
    ++length;

  if (state_save_int32(RSID_MONSTERLIST) ||
      state_save_int32(length))
    return 1;

  for (ptr = mlist; ptr != NULL; ptr = ptr->l_next)
    if (state_save_thing(ptr))
      return 1;

  return 0;
}

bool monster_is_blind(THING *mon)         { return mon->t_flags & ISBLIND; }
bool monster_is_cancelled(THING *mon)     { return mon->t_flags & ISCANC; }
bool monster_is_confused(THING *mon)      { return mon->t_flags & ISHUH; }
bool monster_is_confusing(THING *mon)     { return mon->t_flags & CANHUH; }
bool monster_is_found(THING *mon)         { return mon->t_flags & ISFOUND; }
bool monster_is_hallucinating(THING *mon) { return mon->t_flags & ISHALU; }
bool monster_is_invisible(THING *mon)     { return mon->t_flags & ISINVIS; }
bool monster_is_levitating(THING *mon)    { return mon->t_flags & ISLEVIT; }
bool monster_is_true_seeing(THING *mon)   { return mon->t_flags & CANSEE; }

void monster_set_blind(THING *mon)        { mon->t_flags |= ISBLIND; }
void monster_set_cancelled(THING *mon)    { mon->t_flags |= ISCANC; }
void monster_set_confused(THING *mon)     { mon->t_flags |= ISHUH; }
void monster_set_confusing(THING *mon)    { mon->t_flags |= CANHUH; }
void monster_set_found(THING *mon)        { mon->t_flags |= ISFOUND; }
void monster_set_hallucinating(THING *mon){ mon->t_flags |= ISHALU; }
void monster_set_invisible(THING *mon)
{
  mon->t_flags |= ISINVIS;
  if (cansee(mon->t_pos.y, mon->t_pos.x))
  {
    msg("%s disappeared", set_mname(mon));
    mvaddcch(mon->t_pos.y, mon->t_pos.x, mon->t_oldch);
  }
}
void monster_set_levitating(THING *mon)   { mon->t_flags |= ISLEVIT; }
void monster_set_true_seeing(THING *mon)  { mon->t_flags |= CANSEE; }
void monster_become_held(THING *mon)
{
  mon->t_flags &= ~ISRUN;
  mon->t_flags |= ISHELD;
}
void monster_become_stuck(THING *mon)      { mon->t_flags |= ISSTUCK; }



void monster_remove_blind(THING *mon)        { mon->t_flags &= ~ISBLIND; }
void monster_remove_cancelled(THING *mon)    { mon->t_flags &= ~ISCANC; }
void monster_remove_confused(THING *mon)     { mon->t_flags &= ~ISHUH; }
void monster_remove_confusing(THING *mon)    { mon->t_flags &= ~CANHUH; }
void monster_remove_found(THING *mon)        { mon->t_flags &= ~ISFOUND; }
void monster_remove_hallucinating(THING *mon){ mon->t_flags &= ~ISHALU; }
void monster_remove_invisible(THING *mon)    { mon->t_flags &= ~ISINVIS; }
void monster_remove_levitating(THING *mon)   { mon->t_flags &= ~ISLEVIT; }
void monster_remove_true_seeing(THING *mon)  { mon->t_flags &= ~CANSEE; }
void monster_remove_held(THING *mon)         { mon->t_flags &= ~ISHELD; }



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

  list_attach(&mlist, tp);
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
  THING *tp = allocate_new_item();
  static coord cp;

  do
  {
    room_find_floor((struct room *) NULL, &cp, false, true);
  } while (roomin(&cp) == player_get_room());

  monster_new(tp, monster_random(true), &cp);
  if (player_can_sense_monsters())
  {
    if (player_is_hallucinating())
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

  assert_attached(mlist, tp);
  ch = tp->t_type;

  /* Every time he sees mean monster, it might start chasing him */
  if (!on(*tp, ISRUN) && rnd(3) != 0 && on(*tp, ISMEAN) && !on(*tp, ISHELD)
      && !player_has_ring_with_ability(R_STEALTH) && !player_is_levitating())
  {
    tp->t_dest = player_pos;
    if (!on(*tp, ISSTUCK))
      tp->t_flags |= ISRUN;
  }

  if (ch == 'M' && !player_is_blind() && !player_is_hallucinating()
      && !monster_is_found(tp) && !monster_is_cancelled(tp) && on(*tp, ISRUN))
  {
    struct room *rp = player_get_room();
    if ((rp != NULL && !(rp->r_flags & ISDARK))
        || dist(y, x, player_pos->y, player_pos->x) < LAMPDIST)
    {
      monster_set_found(tp);
      if (!player_save_throw(VS_MAGIC))
      {
        char *mname = set_mname(tp);
        addmsg("%s", mname);
        if (strcmp(mname, "it") != 0)
          addmsg("'");
        msg("s gaze has confused you. ");
        player_set_confused(false);
      }
    }
  }

  /* Let greedy ones guard gold */
  if (on(*tp, ISGREED) && !on(*tp, ISRUN))
  {
    tp->t_flags |= ISRUN;
    if (player_get_room()->r_goldval)
      tp->t_dest = &player_get_room()->r_gold;
    else
      tp->t_dest = player_pos;
  }
  return tp;
}

void
monster_give_pack(THING *tp)
{
  if (level >= max_level && rnd(100) < monsters[tp->t_type-'A'].m_carry)
    list_attach(&tp->t_pack, new_thing());
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
  assert_attached(mlist, tp);

  tp->t_dest = monster_destination(tp);

  if (tp->t_flags & ISSTUCK)
    return;

  /* Start the beastie running */
  tp->t_flags |= ISRUN;
  tp->t_flags &= ~ISHELD;
}

coord *
monster_destination(THING *tp)
{
  THING *obj;
  int prob = monsters[tp->t_type - 'A'].m_carry;

  if (prob <= 0 || tp->t_room == player_get_room() || see_monst(tp))
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
      player_remove_held();
      vf_hit = 0;
      break;

    /* Leprechauns drop gold */
    case 'L':
      {
        THING *gold;

        if (fallpos(&tp->t_pos, &tp->t_room->r_gold) && level >= max_level)
        {
          gold = allocate_new_item();
          gold->o_type = GOLD;
          gold->o_goldval = GOLDCALC;
          if (player_save_throw(VS_MAGIC))
            gold->o_goldval += GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
          list_attach(&tp->t_pack, gold);
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
    list_detach(&tp->t_pack, obj);
    if (waskill)
      fall(obj, false);
    else
      _discard(&obj);
  }

  moat(mp->y, mp->x) = NULL;
  mvaddcch(mp->y, mp->x, tp->t_oldch);
  list_detach(&mlist, tp);

  if (on(*tp, ISTARGET))
  {
    kamikaze = false;
    to_death = false;
    if (fight_flush)
      flushinp();
  }

  _discard(&tp);
}

bool
monster_is_dead(THING *monster)
{
  THING *ptr = mlist;

  if (monster == NULL)
    return true;

  while (ptr != NULL && ptr != monster)
    ptr = ptr->l_next;

  return ptr != monster;
}

void
monster_teleport(THING *monster, coord *destination)
{
  coord new_pos;

  /* Select destination */
  if (destination == NULL)
    do
      room_find_floor(NULL, &new_pos, false, true);
    while (same_coords(new_pos, monster->t_pos));
  else
  {
    new_pos.y = destination->y;
    new_pos.x = destination->x;
  }
  destination = NULL; /* Past this point, use new_pos instead of destination */

  /* Remove monster */
  if (see_monst(monster))
    mvaddcch(monster->t_pos.y, monster->t_pos.x, monster->t_oldch);
  set_oldch(monster, &new_pos);
  moat(monster->t_pos.y, monster->t_pos.x) = NULL;

  /* Add monster */
  monster->t_room = roomin(&new_pos);
  monster->t_pos = new_pos;
  monster_remove_held(monster);

  if (see_monst(monster))
    mvaddcch(new_pos.y, new_pos.x, monster->t_disguise);
  else if (player_can_sense_monsters())
    mvaddcch(new_pos.y, new_pos.x, monster->t_type | A_STANDOUT);
}
