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
#include "armor.h"
#include "rip.h"
#include "rogue.h"

#include "monster.h"
#include "monster_private.h"

THING* mlist = NULL;
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
  THING const* ptr;
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

bool monster_is_blind(THING const* mon)         { return mon->t_flags & ISBLIND; }
bool monster_is_cancelled(THING const* mon)     { return mon->t_flags & ISCANC; }
bool monster_is_confused(THING const* mon)      { return mon->t_flags & ISHUH; }
bool monster_is_confusing(THING const* mon)     { return mon->t_flags & CANHUH; }
bool monster_is_found(THING const* mon)         { return mon->t_flags & ISFOUND; }
bool monster_is_hallucinating(THING const* mon) { return mon->t_flags & ISHALU; }
bool monster_is_invisible(THING const* mon)     { return mon->t_flags & ISINVIS; }
bool monster_is_levitating(THING const* mon)    { return mon->t_flags & ISLEVIT; }
bool monster_is_true_seeing(THING const* mon)   { return mon->t_flags & CANSEE; }
bool monster_is_held(THING const* mon)          { return mon->t_flags & ISHELD; }
bool monster_is_stuck(THING const* mon)         { return mon->t_flags & ISSTUCK; }
bool monster_is_chasing(THING const* mon)       { return mon->t_flags & ISRUN; }
bool monster_is_mean(THING const* mon)          { return mon->t_flags & ISMEAN; }
bool monster_is_greedy(THING const* mon)        { return mon->t_flags & ISGREED; }

void monster_set_blind(THING* mon)        { mon->t_flags |= ISBLIND; }
void monster_set_cancelled(THING* mon)    { mon->t_flags |= ISCANC; }
void monster_set_confused(THING* mon)     { mon->t_flags |= ISHUH; }
void monster_set_confusing(THING* mon)    { mon->t_flags |= CANHUH; }
void monster_set_found(THING* mon)        { mon->t_flags |= ISFOUND; }
void monster_set_hallucinating(THING* mon){ mon->t_flags |= ISHALU; }
void monster_set_invisible(THING* mon)
{
  mon->t_flags |= ISINVIS;
  if (cansee(mon->t_pos.y, mon->t_pos.x))
  {
    msg("%s disappeared", set_mname(mon));
    mvaddcch(mon->t_pos.y, mon->t_pos.x, mon->t_oldch);
  }
}
void monster_set_levitating(THING* mon)   { mon->t_flags |= ISLEVIT; }
void monster_set_true_seeing(THING* mon)  { mon->t_flags |= CANSEE; }
void monster_become_held(THING* mon)
{
  mon->t_flags &= ~ISRUN;
  mon->t_flags |= ISHELD;
}
void monster_become_stuck(THING* mon)      { mon->t_flags |= ISSTUCK; }
void monster_start_chasing(THING* mon)     { mon->t_flags |= ISRUN; }
void monster_set_target(THING* mon, coord* target) { mon->t_dest = target; }



void monster_remove_blind(THING* mon)        { mon->t_flags &= ~ISBLIND; }
void monster_remove_cancelled(THING* mon)    { mon->t_flags &= ~ISCANC; }
void monster_remove_confused(THING* mon)     { mon->t_flags &= ~ISHUH; }
void monster_remove_confusing(THING* mon)    { mon->t_flags &= ~CANHUH; }
void monster_remove_found(THING* mon)        { mon->t_flags &= ~ISFOUND; }
void monster_remove_hallucinating(THING* mon){ mon->t_flags &= ~ISHALU; }
void monster_remove_invisible(THING* mon)    { mon->t_flags &= ~ISINVIS; }
void monster_remove_levitating(THING* mon)   { mon->t_flags &= ~ISLEVIT; }
void monster_remove_true_seeing(THING* mon)  { mon->t_flags &= ~CANSEE; }
void monster_remove_held(THING* mon)         { mon->t_flags &= ~ISHELD; }



char
monster_random(bool wander)
{
  /* List of monsters in rough order of vorpalness */
  char const* lvl_mons =  "KEBSHIROZLCQANYFTWPXUMVGJD";
  char const* wand_mons = "KEBSH0ROZ0CQA0Y0TWP0UMVGJ0";
  char const* mons = (wander ? wand_mons : lvl_mons);

  int d;
  do
  {
    d = level + (rnd(10) - 6);
    if (d < 0)
      d = rnd(5);
    if (d > 25)
      d = rnd(5) + 21;
  }
  while (mons[d] != 0);

  return mons[d];
}

/** monster_xp_worth
 * Experience to add for this monster's level/hit points */
static int
monster_xp_worth(THING* tp)
{
  int mod = tp->t_stats.s_lvl == 1
    ? tp->t_stats.s_maxhp / 8
    : tp->t_stats.s_maxhp / 6;

  if (tp->t_stats.s_lvl > 9)
    mod *= 20;
  else if (tp->t_stats.s_lvl > 6)
    mod *= 4;

  return mod;
}

void
monster_new(THING* monster, char type, coord* pos)
{
  int lev_add = level - AMULETLEVEL;
  if (lev_add < 0)
    lev_add = 0;

  list_attach(&mlist, monster);
  monster->t_type       = type;
  monster->t_disguise   = type;
  monster->t_pos        = *pos;
  monster->t_oldch      = mvincch(pos->y, pos->x);
  monster->t_room       = roomin(pos);
  moat(pos->y, pos->x)  = monster;

  struct monster const* template = &monsters[monster->t_type-'A'];
  monster->t_stats.s_lvl   = template->m_stats.s_lvl + lev_add;
  monster->t_stats.s_hpt   = roll(monster->t_stats.s_lvl, 8);
  monster->t_stats.s_maxhp = monster->t_stats.s_hpt;
  monster->t_stats.s_arm   = template->m_stats.s_arm - lev_add;
  monster->t_stats.s_str   = template->m_stats.s_str;
  monster->t_stats.s_exp   = template->m_stats.s_exp + lev_add * 10 +
                             monster_xp_worth(monster);
  monster->t_turn          = true;
  monster->t_pack          = NULL;
  strcpy(monster->t_stats.s_dmg,template->m_stats.s_dmg);
  monster->t_flags         = template->m_flags;

  if (level > 29)
    monster->t_flags |= ISHASTE;

  if (player_has_ring_with_ability(R_AGGR))
    monster_start_running(pos);

  if (type == 'X')
    monster->t_disguise = rnd_thing();
}

void
monster_new_random_wanderer(void)
{
  coord monster_pos;

  do
    room_find_floor((struct room *) NULL, &monster_pos, false, true);
  while (roomin(&monster_pos) == player_get_room());

  THING* monster = allocate_new_item();
  monster_new(monster, monster_random(true), &monster_pos);
  if (player_can_sense_monsters())
  {
    if (player_is_hallucinating())
      addcch((rnd(26) + 'A') | A_STANDOUT);
    else
      addcch(monster->t_type | A_STANDOUT);
  }
  monster_start_running(&monster->t_pos);
}

THING*
monster_notice_player(int y, int x)
{
  THING *monster = moat(y, x);

  assert_attached(mlist, monster);

  coord* player_pos = player_get_pos();

  /* Monster can begin chasing after the player if: */
  if (!monster_is_chasing(monster)
      && monster_is_mean(monster)
      && !monster_is_held(monster)
      && !player_is_stealthy()
      && !rnd(3))
  {
    monster_set_target(monster, player_pos);
    if (!monster_is_stuck(monster))
      monster_start_chasing(monster);
  }

  /* Medusa can confuse player */
  if (monster->t_type == 'M'
      && !player_is_blind()
      && !player_is_hallucinating()
      && !monster_is_found(monster)
      && !monster_is_cancelled(monster)
      && monster_is_chasing(monster))
  {
    struct room const* rp = player_get_room();
    if ((rp != NULL && !(rp->r_flags & ISDARK))
        || dist(y, x, player_pos->y, player_pos->x) < LAMPDIST)
    {
      monster_set_found(monster);
      if (!player_save_throw(VS_MAGIC))
      {
        char const* mname = set_mname(monster);
        msg("%s's gaze has confused you", mname);
        player_set_confused(false);
      }
    }
  }

  /* Let greedy ones guard gold */
  if (monster_is_greedy(monster) && !monster_is_chasing(monster))
  {
    monster_set_target(monster, player_get_room()->r_goldval
        ? &player_get_room()->r_gold
        : player_pos);
    monster_start_chasing(monster);
  }

  return monster;
}

void
monster_give_pack(THING* tp)
{
  if (level >= max_level
      && rnd(100) < monsters[tp->t_type-'A'].m_carry)
    list_attach(&tp->t_pack, new_thing());
}

int
monster_save_throw(int which, THING const* tp)
{
  int need = 14 + which - tp->t_stats.s_lvl / 2;
  return (roll(1, 20) >= need);
}

void
monster_start_running(coord* runner)
{
  THING *tp = moat(runner->y, runner->x);
  assert_attached(mlist, tp);

  monster_find_new_target(tp);
  if (!monster_is_stuck(tp))
  {
    monster_remove_held(tp);
    monster_start_chasing(tp);
  }
}

void
monster_on_death(THING* monster, bool pr)
{
  if (game_type == DEFAULT)
    player_earn_exp(monster->t_stats.s_exp);

  switch (monster->t_type)
  {
    /* If the monster was a venus flytrap, un-hold him */
    case 'F':
      player_remove_held();
      vf_hit = 0;
      break;

    /* Leprechauns drop gold */
    case 'L':
      if (fallpos(&monster->t_pos, &monster->t_room->r_gold))
      {
        THING* gold = allocate_new_item();
        gold->o_type = GOLD;
        gold->o_goldval = GOLDCALC;
        if (player_save_throw(VS_MAGIC))
          gold->o_goldval += GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
        list_attach(&monster->t_pack, gold);
      }
  }

  /* Get rid of the monster. */
  char const* mname = set_mname(monster);
  monster_remove_from_screen(&monster->t_pos, monster, true);
  if (pr)
    msg("you have slain %s", mname);

  /* Do adjustments if he went up a level */
  player_check_for_level_up();
  if (fight_flush)
    flushinp();
}

void
monster_remove_from_screen(coord* mp, THING* tp, bool waskill)
{
  THING* nexti;
  for (THING* obj = tp->t_pack; obj != NULL; obj = nexti)
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
monster_is_dead(THING const* monster)
{
  if (monster == NULL)
    return true;

  for (THING const* ptr = mlist; ptr != NULL; ptr = ptr->l_next)
    if (ptr == monster)
      return false;

  return true;
}

void
monster_teleport(THING* monster, coord* destination)
{
  /* Select destination */
  coord new_pos;
  if (destination == NULL)
    do
      room_find_floor(NULL, &new_pos, false, true);
    while (same_coords(&new_pos, &monster->t_pos));
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

void
monster_do_special_ability(THING** monster)
{
  assert(*monster != NULL);

  if (monster_is_cancelled(*monster))
    return;

  switch ((*monster)->t_type)
  {
    /* If an aquator hits, you can lose armor class */
    case 'A':
      armor_rust();
      return;

    /* Venus Flytrap stops the poor guy from moving */
    case 'F':
      player_set_held();
      ++vf_hit;
      player_lose_health(1);
      if (player_get_health() <= 0)
        death('F');
      return;

    /* The ice monster freezes you */
    case 'I':
      player_stop_running();
      if (!no_command)
        msg("you are frozen by the %s", set_mname(*monster));
      no_command += rnd(2) + 2;
      if (no_command > 50)
        death('h');
      return;


    /* Leperachaun steals some gold and disappears */
    case 'L':
      monster_remove_from_screen(&(*monster)->t_pos, *monster, false);
      *monster = NULL;

      purse -= GOLDCALC;
      if (!player_save_throw(VS_MAGIC))
        purse -= GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
      if (purse < 0)
        purse = 0;
      msg("your purse feels lighter");
      return;


    /* Nymph's steal a magic item and disappears */
    case 'N':;
      THING* steal = pack_find_magic_item();
      if (steal != NULL)
      {
        monster_remove_from_screen(&(*monster)->t_pos, *monster, false);
        *monster = NULL;
        pack_remove(steal, false, false);
        msg("your pack feels lighter");
        _discard(&steal);
      }
      return;

    /* Rattlesnakes have poisonous bites */
    case 'R':
      if (!player_save_throw(VS_POISON)
          && !player_has_ring_with_ability(R_SUSTSTR))
      {
        player_modify_strength(-1);
        msg("you feel weaker");
      }
      return;

    /* Vampires can steal max hp */
    case 'V':
      if (rnd(100) < 30)
      {
        int fewer = roll(1, 3);
        player_lose_health(fewer);
        player_modify_max_health(-fewer);
        if (player_get_health() <= 0)
          death('V');
        msg("you feel weaker");
      }
      return;

    /* Wraiths might drain exp */
    case 'W':
      if (rnd(100) < 15)
      {
        if (player_get_exp() == 0)
          death('W');  /* Death by no exp */
        player_lower_level();

        int fewer = roll(1, 10);
        player_lose_health(fewer);
        player_modify_max_health(-fewer);
        if (player_get_health() <= 0)
          death('W');
        msg("you feel weaker");
      }
      return;

    default: return;
  }
}
