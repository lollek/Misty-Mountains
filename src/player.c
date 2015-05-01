#include "rogue.h"

#include "pack.h"
#include "rings.h"
#include "misc.h"
#include "io.h"

#include "player.h"

static const int player_min_strength = 3;
static const int player_max_strength = 31;

static str_t
player_get_strength_bonuses(void)
{
  int i;
  str_t bonuses = 0;
  for (i = 0; i < RING_SLOTS_SIZE; ++i)
  {
    THING *ring = pack_equipped_item(ring_slots[i]);
    if (ring != NULL && ring->o_which == R_ADDSTR)
      bonuses += ring->o_arm;
  }
  return bonuses;
}

static void
player_update_max_strength(void)
{
  str_t bonuses = player_get_strength_bonuses();
  if (player.t_stats.s_str - bonuses > max_stats.s_str)
    max_stats.s_str = player.t_stats.s_str - bonuses;
}

coord *
player_get_pos(void)
{
  return &player.t_pos;
}

void
player_set_pos(coord *new_pos)
{
  player.t_pos.x = new_pos->x;
  player.t_pos.y = new_pos->y;
}

int
player_get_strength(void)
{
  return player.t_stats.s_str;
}

bool
player_strength_is_weakened(void)
{
  return player.t_stats.s_str < max_stats.s_str;
}

void
player_restore_strength(void)
{
  player.t_stats.s_str = max_stats.s_str + player_get_strength_bonuses();
}

void
player_modify_strength(int amount)
{
  player.t_stats.s_str += amount;
  if (player.t_stats.s_str < 3)
    player.t_stats.s_str = 3;
  else if (player.t_stats.s_str > 31)
    player.t_stats.s_str = 31;

  player_update_max_strength();
}

int
player_get_health(void)
{
  return player.t_stats.s_hpt;
}

int
player_get_max_health(void)
{
  return player.t_stats.s_maxhp;
}

void
player_restore_health(int amount, bool can_raise_total)
{
  player.t_stats.s_hpt += amount;

  if (can_raise_total)
  {
    int amount = 0;
    if (player_get_health() > player_get_max_health() + player_get_level() + 1)
      ++amount;
    if (player_get_health() > player_get_max_health())
      ++amount;
    if (amount > 0)
      player_modify_max_health(amount);
  }

  if (player_get_health() > player_get_max_health())
    player.t_stats.s_hpt = player_get_max_health();
}

bool
player_is_hurt(void)
{
  return player_get_health() != player_get_max_health();
}

void
player_modify_max_health(int amount)
{
  player.t_stats.s_maxhp += amount;
  if (player.t_stats.s_hpt > player.t_stats.s_maxhp)
    player.t_stats.s_hpt = player.t_stats.s_maxhp;
}

void player_lose_health(int amount)
{
  player.t_stats.s_hpt -= amount;
}

int
player_get_level(void)
{
  return player.t_stats.s_lvl;
}

void
player_raise_level(void)
{
  player.t_stats.s_exp = e_levels[player.t_stats.s_lvl-1] + 1L;
  player_check_for_level_up();
}

void
player_check_for_level_up(void)
{
  int i;
  int old_level;

  for (i = 0; e_levels[i] != 0; ++i)
    if (e_levels[i] > player.t_stats.s_exp)
      break;

  ++i;
  old_level = player.t_stats.s_lvl;
  player.t_stats.s_lvl = i;

  if (i > old_level)
  {
    int add_to_hp = roll(i - old_level, 10);
    player_modify_max_health(add_to_hp);
    player_restore_health(add_to_hp, false);
    msg("welcome to level %d", player.t_stats.s_lvl);
  }
}

void
player_lower_level(void)
{
  --player.t_stats.s_lvl;
  if (player.t_stats.s_lvl == 0)
  {
    player.t_stats.s_exp = 0;
    player.t_stats.s_lvl = 1;
  }
  else
    player.t_stats.s_exp = e_levels[player.t_stats.s_lvl-1] +1L;
}

int player_get_exp(void)
{
  return player.t_stats.s_exp;
}

void player_earn_exp(int amount)
{
  player.t_stats.s_exp += amount;
}
