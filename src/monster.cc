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

#include <string>

using namespace std;

#include "io.h"
#include "pack.h"
#include "scrolls.h"
#include "rings.h"
#include "rooms.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "weapons.h"
#include "things.h"
#include "os.h"
#include "armor.h"
#include "options.h"
#include "rogue.h"
#include "death.h"
#include "passages.h"

#include "monster.h"
#include "monster_private.h"

int    monster_flytrap_hit = 0; /* Number of time flytrap has hit */

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

void
monster_set_invisible(monster* mon)
{
  assert(mon != nullptr);

  mon->t_flags |= ISINVIS;
  if (cansee(mon->t_pos.y, mon->t_pos.x))
  {
    char buf[MAXSTR];
    io_msg("%s disappeared", monster_name(mon, buf));
    mvaddcch(mon->t_pos.y, mon->t_pos.x, static_cast<chtype>(mon->t_oldch));
  }
}

void
monster_become_held(monster* mon)
{
  assert(mon != nullptr);

  mon->t_flags &= ~ISRUN;
  mon->t_flags |= ISHELD;
}



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
    d = level + (os_rand_range(10) - 6);
    if (d < 0)
      d = os_rand_range(5);
    if (d > 25)
      d = os_rand_range(5) + 21;
  }
  while (mons[d] == 0);

  return mons[d];
}

/** monster_xp_worth
 * Experience to add for this monster's level/hit points */
static int
monster_xp_worth(monster* tp)
{
  assert(tp != nullptr);

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
monster_new(monster* monster, char type, Coordinate* pos)
{
  assert(monster != nullptr);
  assert(pos != nullptr);

  monster_list.push_back(monster);
  monster->t_type       = type;
  monster->t_disguise   = type;
  monster->t_pos        = *pos;
  monster->t_oldch      = static_cast<char>(mvincch(pos->y, pos->x));
  monster->t_room       = roomin(pos);
  level_set_monster(pos->y, pos->x, monster);

  struct monster_template const* m_template = &monsters[monster->t_type - 'A'];
  struct stats* new_stats = &monster->t_stats;

  new_stats->s_lvl   = m_template->m_level;
  new_stats->s_hpt   = roll(new_stats->s_lvl, 8);
  new_stats->s_maxhp = new_stats->s_hpt;
  new_stats->s_arm   = m_template->m_armor;
  new_stats->s_str   = 10;
  new_stats->s_exp   = m_template->m_basexp + monster_xp_worth(monster);
  assert(sizeof(new_stats->s_dmg) == sizeof(m_template->m_dmg));
  memcpy(new_stats->s_dmg, m_template->m_dmg, sizeof(m_template->m_dmg));

  monster->t_turn          = true;
  monster->t_flags         = m_template->m_flags;

  if (player_has_ring_with_ability(R_AGGR))
    monster_start_running(pos);

  if (type == 'X')
    monster->t_disguise = rnd_thing();
}

void
monster_new_random_wanderer(void)
{
  Coordinate monster_pos;

  do
    room_find_floor(nullptr, &monster_pos, false, true);
  while (roomin(&monster_pos) == player_get_room());

  monster* monster = new struct monster();
  monster_new(monster, monster_random(true), &monster_pos);
  if (player_can_sense_monsters())
  {
    if (player_is_hallucinating())
      addcch(static_cast<chtype>(os_rand_range(26) + 'A') | A_STANDOUT);
    else
      addcch(static_cast<chtype>(monster->t_type) | A_STANDOUT);
  }
  monster_start_running(&monster->t_pos);
}

monster*
monster_notice_player(int y, int x)
{
  monster *monster = level_get_monster(y, x);

  Coordinate* player_pos = player_get_pos();

  /* Monster can begin chasing after the player if: */
  if (!monster_is_chasing(monster)
      && monster_is_mean(monster)
      && !monster_is_held(monster)
      && !player_is_stealthy()
      && !os_rand_range(3))
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
    if ((rp != nullptr && !(rp->r_flags & ISDARK))
        || dist(y, x, player_pos->y, player_pos->x) < LAMPDIST)
    {
      monster_set_found(monster);
      if (!player_save_throw(VS_MAGIC))
      {
        char buf[MAXSTR];
        io_msg("%s's gaze has confused you", monster_name(monster, buf));
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
monster_give_pack(monster* mon)
{
  assert(mon != nullptr);

  if (level >= level_max && os_rand_range(100) < monsters[mon->t_type-'A'].m_carry)
    mon->t_pack.push_back(new_thing());
}

int
monster_save_throw(int which, monster const* mon)
{
  assert(mon != nullptr);

  int need = 14 + which - mon->t_stats.s_lvl / 2;
  return (roll(1, 20) >= need);
}

void
monster_start_running(Coordinate const* runner)
{
  monster *tp = level_get_monster(runner->y, runner->x);

  monster_find_new_target(tp);
  if (!monster_is_stuck(tp))
  {
    monster_remove_held(tp);
    monster_start_chasing(tp);
  }
}

void
monster_on_death(monster* monster, bool pr)
{
  assert(monster != nullptr);

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
        Item* gold = new Item();
        gold->o_type = GOLD;
        gold->o_goldval = GOLDCALC;
        if (player_save_throw(VS_MAGIC))
          gold->o_goldval += GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
        monster->t_pack.push_back(gold);
      }
  }

  /* Get rid of the monster. */
  char mname[MAXSTR];
  monster_name(monster, mname);
  monster_remove_from_screen(&monster->t_pos, monster, true);
  if (pr)
    io_msg("you have slain %s", mname);

  /* Do adjustments if he went up a level */
  player_check_for_level_up();
  if (fight_flush)
    flushinp();
}

void
monster_remove_from_screen(Coordinate* mp, monster* tp, bool waskill)
{
  assert(mp != nullptr);
  assert(tp != nullptr);

  for (Item* obj : tp->t_pack) {
    obj->o_pos = tp->t_pos;
    if (waskill)
      weapon_missile_fall(obj, false);
    else
      delete obj;
  }
  tp->t_pack.clear();

  level_set_monster(mp->y, mp->x, nullptr);
  mvaddcch(mp->y, mp->x, static_cast<chtype>(tp->t_oldch));

  monster_list.remove(tp);

  if (monster_is_players_target(tp))
  {
    to_death = false;
    if (fight_flush)
      flushinp();
  }

  delete tp;
}

bool
monster_is_dead(monster const* monster)
{
  if (monster == nullptr)
    return true;

  return !(find(monster_list.cbegin(), monster_list.cend(), monster) ==
          monster_list.cend());
}

void
monster_teleport(monster* monster, Coordinate const* destination)
{
  /* Select destination */
  Coordinate new_pos;
  if (destination == nullptr)
    do
      room_find_floor(nullptr, &new_pos, false, true);
    while (new_pos == monster->t_pos);
  else
    new_pos = *destination;

  /* Remove monster */
  if (monster_seen_by_player(monster))
    mvaddcch(monster->t_pos.y, monster->t_pos.x, static_cast<chtype>(monster->t_oldch));
  set_oldch(monster, &new_pos);
  level_set_monster(monster->t_pos.y, monster->t_pos.x, nullptr);

  /* Add monster */
  monster->t_room = roomin(&new_pos);
  monster->t_pos = new_pos;
  monster_remove_held(monster);

  if (monster_seen_by_player(monster))
    mvaddcch(new_pos.y, new_pos.x, static_cast<chtype>(monster->t_disguise));
  else if (player_can_sense_monsters())
    mvaddcch(new_pos.y, new_pos.x, static_cast<chtype>(monster->t_type)| A_STANDOUT);
}

void
monster_do_special_ability(monster** monster)
{
  assert(monster != nullptr);
  assert(*monster != nullptr);

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
      if (!player_turns_without_action)
      {
        char buf[MAXSTR];
        io_msg("you are frozen by the %s", monster_name(*monster, buf));
      }
      player_turns_without_action += os_rand_range(2) + 2;
      if (player_turns_without_action > 50)
        death(DEATH_ICE);
      return;


    /* Leperachaun steals some gold and disappears */
    case 'L':
      monster_remove_from_screen(&(*monster)->t_pos, *monster, false);
      *monster = nullptr;

      pack_gold -= GOLDCALC;
      if (!player_save_throw(VS_MAGIC))
        pack_gold -= GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
      if (pack_gold < 0)
        pack_gold = 0;
      io_msg("your pack_gold feels lighter");
      return;


    /* Nymph's steal a magic item and disappears */
    case 'N': {
      Item* steal = pack_find_magic_item();
      if (steal != nullptr)
      {
        monster_remove_from_screen(&(*monster)->t_pos, *monster, false);
        *monster = nullptr;
        pack_remove(steal, false, false);
        io_msg("your pack feels lighter");
        delete steal;
      }
      return;
      }

    /* Rattlesnakes have poisonous bites */
    case 'R':
      if (!player_save_throw(VS_POISON)
          && !player_has_ring_with_ability(R_SUSTSTR))
      {
        player_modify_strength(-1);
        io_msg("you feel weaker");
      }
      return;

    /* Vampires can steal max hp */
    case 'V':
      if (os_rand_range(100) < 30)
      {
        int fewer = roll(1, 3);
        player_lose_health(fewer);
        player_modify_max_health(-fewer);
        if (player_get_health() <= 0)
          death('V');
        io_msg("you feel weaker");
      }
      return;

    /* Wraiths might drain exp */
    case 'W':
      if (os_rand_range(100) < 15)
      {
        if (player_get_exp() == 0)
          death('W');  /* Death by no exp */
        player_lower_level();

        int fewer = roll(1, 10);
        player_lose_health(fewer);
        player_modify_max_health(-fewer);
        if (player_get_health() <= 0)
          death('W');
        io_msg("you feel weaker");
      }
      return;

    default: return;
  }
}

char const*
monster_name(monster const* monster, char* buf)
{
  assert(monster != nullptr);
  assert(buf != nullptr);

  if (!monster_seen_by_player(monster) && !player_can_sense_monsters())
    strcpy(buf, "something");

  else if (player_is_hallucinating())
  {
    int ch = mvincch(monster->t_pos.y, monster->t_pos.x);
    if (!isupper(ch))
      ch = static_cast<int>(os_rand_range(NMONSTERS));
    else
      ch -= 'A';

    sprintf(buf, "the %s", monster_name_by_type(static_cast<char>(ch)).c_str());
  }

  else
    sprintf(buf, "the %s", monster_name_by_type(monster->t_type).c_str());

  return buf;
}

string const&
monster_name_by_type(char monster_type)
{
  assert(monster_type >= 'A');
  assert(monster_type < static_cast<char>('A' + NMONSTERS));
  return monsters[monster_type - 'A'].m_name;
}

bool
monster_seen_by_player(monster const* monster)
{
  assert(monster != nullptr);

  Coordinate const* player_pos = player_get_pos();
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
  return !(monster->t_room->r_flags & ISDARK);
}

bool
monster_is_anyone_seen_by_player(void)
{
  for (monster* mon : monster_list) {
    if (monster_seen_by_player(mon)) {
      return true;
    }
  }
  return false;
}

void
monster_show_all_as_trippy(void)
{
  bool seemonst = player_can_sense_monsters();
  for (monster const* tp : monster_list) {

    if (monster_seen_by_player(tp)) {
      chtype symbol = (tp->t_type == 'X' && tp->t_disguise != 'X')
        ? static_cast<chtype>(rnd_thing())
        : static_cast<chtype>(os_rand_range(26) + 'A');
      mvaddcch(tp->t_pos.y, tp->t_pos.x, symbol);
    }
    else if (seemonst) {
      mvaddcch(tp->t_pos.y, tp->t_pos.x,
          static_cast<chtype>(os_rand_range(26) + 'A') | A_STANDOUT);
    }
  }
}

void
monster_move_all(void)
{
  for (monster* mon : monster_list) {

    if (!monster_is_held(mon) && monster_is_chasing(mon))
    {
      bool wastarget = monster_is_players_target(mon);
      Coordinate orig_pos = mon->t_pos;
      if (!monster_chase(mon))
        continue;

      if (monster_is_flying(mon)
          && dist_cp(player_get_pos(), &mon->t_pos) >= 3)
        monster_chase(mon);

      if (wastarget && !(orig_pos == mon->t_pos))
      {
        mon->t_flags &= ~ISTARGET;
        to_death = false;
      }
    }
  }
}

void
monster_remove_all(void)
{
  for (monster* mon : monster_list) {
    delete mon;
  }
  monster_list.clear();
}

void
monster_set_all_rooms(void)
{
  for (monster* mon : monster_list) {
    mon->t_room = roomin(&mon->t_pos);
  }
}

void
monster_aggravate_all(void)
{
  for (monster* mon : monster_list) {
    monster_start_running(&mon->t_pos);
  }
}

void
monster_show_all_hidden(void)
{
  for (monster* mon : monster_list) {
    if (monster_is_invisible(mon) && monster_seen_by_player(mon)
        && !player_is_hallucinating())
      mvaddcch(mon->t_pos.y, mon->t_pos.x, static_cast<chtype>(mon->t_disguise));
  }
}

void
monster_aggro_all_which_desire_item(Item* item)
{
  for (monster* mon : monster_list) {
    if (mon->t_dest == &item->o_pos) {
      mon->t_dest = player_get_pos();
    }
  }
}

void
monster_hide_all_invisible(void)
{
  for (monster* mon : monster_list) {
    if (monster_is_invisible(mon) && monster_seen_by_player(mon)) {
      mvaddcch(mon->t_pos.y, mon->t_pos.x, static_cast<chtype>(mon->t_oldch));
    }
  }
}

bool
monster_sense_all_hidden(void)
{
  bool spotted_something = false;
  for (monster* mon : monster_list) {
    if (!monster_seen_by_player(mon)) {
      mvaddcch(mon->t_pos.y, mon->t_pos.x,
          (player_is_hallucinating()
           ? static_cast<chtype>(os_rand_range(26) + 'A')
           : static_cast<chtype>(mon->t_type))
            | A_STANDOUT);
      spotted_something = true;
    }
  }
  return spotted_something;
}

void
monster_unsense_all_hidden(void)
{
  for (monster* mon : monster_list) {
    if (!monster_seen_by_player(mon)) {
      mvaddcch(mon->t_pos.y, mon->t_pos.x, static_cast<chtype>(mon->t_oldch));
    }
  }
}

void
monster_print_all(void)
{
  for (monster* mon : monster_list) {

    if (cansee(mon->t_pos.y, mon->t_pos.x)) {
      chtype symbol = (!monster_is_invisible(mon) || player_has_true_sight())
        ? static_cast<chtype>(mon->t_disguise)
        : static_cast<chtype>(level_get_ch(mon->t_pos.y, mon->t_pos.x));
      mvaddcch(mon->t_pos.y, mon->t_pos.x, symbol);

    } else if (player_can_sense_monsters()) {
      mvaddcch(mon->t_pos.y, mon->t_pos.x, static_cast<chtype>(mon->t_type)| A_STANDOUT);
    }
  }
}

bool
monster_show_if_magic_inventory(void)
{
  bool atleast_one = false;
  for (monster* mon : monster_list) {
    for (Item* item : mon->t_pack) {
      if (is_magic(item))
      {
        atleast_one = true;
        mvwaddcch(hw, mon->t_pos.y, mon->t_pos.x, MAGIC);
      }
    }
  }
  return atleast_one;
}

int
monster_add_nearby(monster** nearby_monsters, struct room const* room)
{
  assert(nearby_monsters != nullptr);
  bool inpass = player_get_room()->r_flags & ISGONE;
  monster** nearby_monsters_start = nearby_monsters;

  for (monster* mon : monster_list) {
    if (mon->t_room == player_get_room()
        || mon->t_room == room
        ||(inpass && level_get_ch(mon->t_pos.y, mon->t_pos.x) == DOOR &&
          &passages[level_get_flags(mon->t_pos.y, mon->t_pos.x) & F_PNUM]
          == player_get_room())) {
      *nearby_monsters++ = mon;
    }
  }

  return static_cast<int>(nearby_monsters_start - nearby_monsters);
}

void
monster_polymorph(monster* target)
{
  assert(target != nullptr);

  Coordinate pos(target->t_pos.x, target->t_pos.y);

  if (target->t_type == 'F')
    player_remove_held();

  list<Item*> target_pack = target->t_pack;
  monster_list.remove(target);

  bool was_seen = monster_seen_by_player(target);
  if (was_seen)
  {
    mvaddcch(pos.y, pos.x, static_cast<chtype>(level_get_ch(pos.y, pos.x)));
    char buf[MAXSTR];
    io_msg_add("%s", monster_name(target, buf));
  }

  char oldch = target->t_oldch;

  char monster = static_cast<char>(os_rand_range(26) + 'A');
  bool same_monster = monster == target->t_type;

  monster_new(target, monster, &pos);
  if (monster_seen_by_player(target))
  {
    mvaddcch(pos.y, pos.x, static_cast<chtype>(monster));
    if (same_monster)
      io_msg(" now looks a bit different");
    else
    {
      char buf[MAXSTR];
      io_msg(" turned into a %s", monster_name(target, buf));
    }
  }
  else if (was_seen)
    io_msg(" disappeared");

  target->t_oldch = oldch;
  target->t_pack = target_pack;
}
