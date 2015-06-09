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
#include <ctype.h>
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
#include "options.h"
#include "rogue.h"

#include "monster.h"
#include "monster_private.h"

int    monster_flytrap_hit = 0; /* Number of time flytrap has hit */
THING* monster_list = NULL;

#define NMONSTERS sizeof(monsters) / sizeof(*monsters)
struct monster_template monsters[] =
{
  /* Name        CARRY  FLAG                   exp lvl  amr  dmg              */
  { "aquator",       0, ISMEAN,                 20,  5,  18, {{0,1}}},
  { "bat",           0, ISFLY,                   1,  1,  17, {{1,2}}},
  { "centaur",      15, 0,                      17,  4,  16, {{1,2},{1,5},{1,5}}},
  { "dragon",      100, ISMEAN,               5000, 10,  21, {{1,8},{1,8},{3,10}}},
  { "emu",           0, ISMEAN,                  2,  1,  13, {{1,2}}},
  { "venus flytrap", 0, ISMEAN,                 80,  8,  17, {{0,1}}},
  { "griffin",      20, ISMEAN|ISFLY|ISREGEN, 2000, 13,  18, {{4,3},{3,5}}},
  { "hobgoblin",     0, ISMEAN,                  3,  1,  15, {{1,8}}},
  { "ice monster",   0, 0,                       5,  1,  11, {{0,1}}},
  { "jabberwock",   70, 0,                    3000, 15,  14, {{2,12},{2,4}}},
  { "kestrel",       0, ISMEAN|ISFLY,            1,  1,  13, {{1,4}}},
  { "leprechaun",    0, 0,                      10,  3,  12, {{1,1}}},
  { "medusa",       40, ISMEAN,                200,  8,  18, {{3,4},{3,4},{2,5}}},
  { "nymph",       100, 0,                      37,  3,  11, {{0,1}}},
  { "orc",          15, ISGREED,                 5,  1,  14, {{1,8}}},
  { "phantom",       0, ISINVIS,               120,  8,  17, {{4,4}}},
  { "quagga",        0, ISMEAN,                 15,  3,  17, {{1,5},{1,5}}},
  { "rattlesnake",   0, ISMEAN,                  9,  2,  17, {{1,6}}},
  { "snake",         0, ISMEAN,                  2,  1,  15, {{1,3}}},
  { "troll",        50, ISREGEN|ISMEAN,        120,  6,  16, {{1,8},{1,8},{2,6}}},
  { "black unicorn", 0, ISMEAN,                190,  7,  22, {{1,9},{1,9},{2,9}}},
  { "vampire",      20, ISREGEN|ISMEAN,        350,  8,  19, {{1,10}}},
  { "wraith",        0, 0,                      55,  5,  16, {{1,6}}},
  { "xeroc",        30, 0,                     100,  7,  13, {{4,4}}},
  { "yeti",         30, 0,                      50,  4,  14, {{1,6},{1,6}}},
  { "zombie",        0, ISMEAN,                  6,  2,  12, {{1,8}}},
};

bool
monsters_save_state(void)
{
  THING const* ptr;
  int length;

  for (ptr = monster_list, length = 0; ptr != NULL; ptr = ptr->l_next)
    ++length;

  if (state_save_int32(RSID_MONSTERLIST) ||
      state_save_int32(length))
    return 1;

  for (ptr = monster_list; ptr != NULL; ptr = ptr->l_next)
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
bool monster_is_players_target(THING const* mon){ return mon->t_flags & ISTARGET;}
bool monster_is_slow(THING const* mon)          { return mon->t_flags & ISSLOW; }
bool monster_is_hasted(THING const* mon)        { return mon->t_flags & ISHASTE; }
bool monster_is_flying(THING const* mon)        { return mon->t_flags & ISFLY; }

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
    char buf[MAXSTR];
    msg("%s disappeared", monster_name(mon, buf));
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
  while (mons[d] == 0);

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
  list_attach(&monster_list, monster);
  monster->t_type       = type;
  monster->t_disguise   = type;
  monster->t_pos        = *pos;
  monster->t_oldch      = mvincch(pos->y, pos->x);
  monster->t_room       = roomin(pos);
  level_set_monster(pos->y, pos->x, monster);

  struct monster_template const* template = &monsters[monster->t_type - 'A'];
  struct stats* new_stats = &monster->t_stats;

  new_stats->s_lvl   = template->m_level;
  new_stats->s_hpt   = roll(new_stats->s_lvl, 8);
  new_stats->s_maxhp = new_stats->s_hpt;
  new_stats->s_arm   = template->m_armor;
  new_stats->s_str   = 10;
  new_stats->s_exp   = template->m_basexp + monster_xp_worth(monster);
  assert(sizeof(new_stats->s_dmg) == sizeof(template->m_dmg));
  memcpy(new_stats->s_dmg, template->m_dmg, sizeof(template->m_dmg));

  monster->t_turn          = true;
  monster->t_pack          = NULL;
  monster->t_flags         = template->m_flags;

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
      addcch((chtype)(rnd(26) + 'A') | A_STANDOUT);
    else
      addcch(monster->t_type | A_STANDOUT);
  }
  monster_start_running(&monster->t_pos);
}

THING*
monster_notice_player(int y, int x)
{
  THING *monster = level_get_monster(y, x);

  list_assert_monster(monster);

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
        char buf[MAXSTR];
        msg("%s's gaze has confused you", monster_name(monster, buf));
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
  if (level >= level_max
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
monster_start_running(coord const* runner)
{
  THING *tp = level_get_monster(runner->y, runner->x);
  list_assert_monster(tp);

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
  player_earn_exp(monster->t_stats.s_exp);

  switch (monster->t_type)
  {
    /* If the monster was a venus flytrap, un-hold him */
    case 'F':
      player_remove_held();
      monster_flytrap_hit = 0;
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
  char mname[MAXSTR];
  monster_name(monster, mname);
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
      os_remove_thing(&obj);
  }

  level_set_monster(mp->y, mp->x, NULL);
  mvaddcch(mp->y, mp->x, tp->t_oldch);
  list_detach(&monster_list, tp);

  if (monster_is_players_target(tp))
  {
    to_death = false;
    if (fight_flush)
      flushinp();
  }

  os_remove_thing(&tp);
}

bool
monster_is_dead(THING const* monster)
{
  if (monster == NULL)
    return true;

  for (THING const* ptr = monster_list; ptr != NULL; ptr = ptr->l_next)
    if (ptr == monster)
      return false;

  return true;
}

void
monster_teleport(THING* monster, coord const* destination)
{
  /* Select destination */
  coord new_pos;
  if (destination == NULL)
    do
      room_find_floor(NULL, &new_pos, false, true);
    while (coord_same(&new_pos, &monster->t_pos));
  else
    new_pos = *destination;

  /* Remove monster */
  if (monster_seen_by_player(monster))
    mvaddcch(monster->t_pos.y, monster->t_pos.x, monster->t_oldch);
  set_oldch(monster, &new_pos);
  level_set_monster(monster->t_pos.y, monster->t_pos.x, NULL);

  /* Add monster */
  monster->t_room = roomin(&new_pos);
  monster->t_pos = new_pos;
  monster_remove_held(monster);

  if (monster_seen_by_player(monster))
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
      ++monster_flytrap_hit;
      player_lose_health(1);
      if (player_get_health() <= 0)
        death('F');
      return;

    /* The ice monster freezes you */
    case 'I':
      player_stop_running();
      if (!no_command)
      {
        char buf[MAXSTR];
        msg("you are frozen by the %s", monster_name(*monster, buf));
      }
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
        os_remove_thing(&steal);
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

char const*
monster_name(THING const* monster, char* buf)
{
  assert(monster != NULL);
  assert(buf != NULL);

  if (!monster_seen_by_player(monster) && !player_can_sense_monsters())
    strcpy(buf, "something");

  else if (player_is_hallucinating())
  {
    int ch = mvincch(monster->t_pos.y, monster->t_pos.x);
    if (!isupper(ch))
      ch = rnd(NMONSTERS);
    else
      ch -= 'A';

    sprintf(buf, "the %s", monster_name_by_type((char)ch));
  }

  else
    sprintf(buf, "the %s", monster_name_by_type(monster->t_type));

  return buf;
}

char const*
monster_name_by_type(char monster_type)
{
  assert(monster_type >= 'A');
  assert(monster_type < 'A' + NMONSTERS);
  return monsters[monster_type - 'A'].m_name;
}

bool
monster_seen_by_player(THING const* monster)
{
  coord const* player_pos = player_get_pos();
  int monster_y = monster->t_pos.y;
  int monster_x = monster->t_pos.x;

  if (player_is_blind() ||
      (monster_is_invisible(monster) && !player_has_true_sight()))
    return false;

  if (dist(monster_y, monster_x, player_pos->y, player_pos->x) < LAMPDIST)
  {
    if (monster_y != player_pos->y && monster_x != player_pos->x
        && !step_ok(level_get_ch(monster_y, player_pos->x))
        && !step_ok(level_get_ch(player_pos->y, monster_x)))
      return false;
    return true;
  }

  if (monster->t_room != player_get_room())
    return false;
  return ((bool)!(monster->t_room->r_flags & ISDARK));
}


