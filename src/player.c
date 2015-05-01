#include <stdbool.h>

#include "pack.h"
#include "rings.h"
#include "misc.h"
#include "io.h"
#include "daemons.h"
#include "status_effects.h"
#include "list.h"
#include "armor.h"
#include "level.h"
#include "rooms.h"
#include "monster.h"
#include "rogue.h"

#include "player.h"

static THING player;

static const int player_min_strength = 3;
static const int player_max_strength = 31;

void *__player_ptr(void) { return &player; }

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

static void
daemon_remove_true_sight(void)
{
  player_remove_true_sight();
}

void
player_init(void)
{
  THING *obj;

  player.t_stats = max_stats;
  food_left = HUNGERTIME;

  /* Give him some food */
  obj = new_item();
  obj->o_type = FOOD;
  obj->o_count = 1;
  pack_add(obj, true);

  /* And his suit of armor */
  obj = new_item();
  obj->o_type = ARMOR;
  obj->o_which = RING_MAIL;
  obj->o_arm = armor_ac(RING_MAIL) - 1;
  obj->o_flags |= ISKNOW;
  obj->o_count = 1;
  pack_equip_item(obj);

  /* Give him his weaponry.  First a mace. */
  obj = new_item();
  init_weapon(obj, MACE);
  obj->o_hplus = 1;
  obj->o_dplus = 1;
  obj->o_flags |= ISKNOW;
  pack_equip_item(obj);

  /* Now a +1 bow */
  obj = new_item();
  init_weapon(obj, BOW);
  obj->o_hplus = 1;
  obj->o_flags |= ISKNOW;
  pack_add(obj, true);

  /* Now some arrows */
  obj = new_item();
  init_weapon(obj, ARROW);
  obj->o_count = rnd(15) + 25;
  obj->o_flags |= ISKNOW;
  pack_add(obj, true);
}

bool
is_player(THING *thing)
{
  return thing == &player;
}

int
player_save_throw(int which)
{
  int need;
  if (which == VS_MAGIC)
  {
    int i;
    for (i = 0; i < RING_SLOTS_SIZE; ++i)
    {
      THING *ring = pack_equipped_item(ring_slots[i]);
      if (ring != NULL && ring->o_which == R_PROTECT)
        which -= ring->o_arm;
    }
  }

  need = 14 + which - player.t_stats.s_lvl / 2;
  return (roll(1, 20) >= need);
}

bool player_has_true_sight(void) { return player.t_flags & CANSEE; }
void player_add_true_sight(bool permanent)
{
  if (player_has_true_sight())
    daemon_lengthen_fuse(daemon_remove_true_sight, SEEDURATION);
  else
  {
    player.t_flags |= CANSEE;
    look(false);
    if (!permanent)
      daemon_start_fuse(daemon_remove_true_sight, 0, SEEDURATION, AFTER);
  }
}
void player_remove_true_sight(void)
{
  /* Hide all invisible monsters */
  THING *monster;
  for (monster = mlist; monster != NULL; monster = monster->l_next)
    if (monster_is_invisible(monster) && see_monst(monster))
      mvaddcch(monster->t_pos.y, monster->t_pos.x, monster->t_oldch);

  /* Set flag */
  player.t_flags &= ~CANSEE;
}

bool player_is_confused(void) { return player.t_flags & ISHUH; }
void player_set_confused(bool permanent)
{
  if (player_is_confused())
    daemon_lengthen_fuse(player_remove_confused, HUHDURATION);
  else
  {
    player.t_flags |= ISHUH;
    look(false);
    if (!permanent)
      daemon_start_fuse(player_remove_confused, 0, HUHDURATION, AFTER);
  }
  msg(player_is_hallucinating()
    ? "what a trippy feeling!"
    : "wait, what's going on here. Huh? What? Who?");
}
void player_remove_confused(void)
{
  player.t_flags &= ~ISHUH;
  msg("you feel less %s now",
    player_is_hallucinating() ? "trippy" : "confused");
}

bool player_is_held(void)     { return player.t_flags & ISHELD; }
void player_set_held(void)    { player.t_flags |= ISHELD; }
void player_remove_held(void) { player.t_flags &= ~ISHELD; }

bool player_can_sense_monsters(void)    { return player.t_flags & SEEMONST; }
void player_add_sense_monsters(bool permanent)
{
  player.t_flags |= SEEMONST;
  if (!permanent)
      daemon_start_fuse((void(*)())turn_see, true, MFINDDURATION, AFTER);
  /* FIXME: Make sure that this work */
  if (!turn_see(false))
    msg("you have a %s feeling for a moment, then it passes",
        player_is_hallucinating() ? "normal" : "strange");
}
void player_remove_sense_monsters(void) { player.t_flags &= ~SEEMONST; }

bool player_is_hallucinating(void)     { return player.t_flags & ISHALU; }
void player_set_hallucinating(bool permanent)
{
  if (player_is_hallucinating())
    daemon_lengthen_fuse(player_remove_hallucinating, SEEDURATION);
  else
  {
    if (player_can_sense_monsters())
      turn_see(false);
    daemon_start(daemon_change_visuals, 0, BEFORE);
    player.t_flags |= ISHALU;
    look(false);
    if (!permanent)
      daemon_start_fuse(player_remove_hallucinating, 0, SEEDURATION, AFTER);
    msg("Oh, wow!  Everything seems so cosmic!");
  }
}
void player_remove_hallucinating(void)
{
  THING *tp;

  if (!player_is_hallucinating())
    return;

  daemon_kill(daemon_change_visuals);
  player.t_flags &= ~ISHALU;

  if (player_is_blind())
    return;

  /* undo the things */
  for (tp = lvl_obj; tp != NULL; tp = tp->l_next)
    if (cansee(tp->o_pos.y, tp->o_pos.x))
      mvaddcch(tp->o_pos.y, tp->o_pos.x, (chtype)tp->o_type);

  /* undo the monsters */
  for (tp = mlist; tp != NULL; tp = tp->l_next)
  {
    move(tp->t_pos.y, tp->t_pos.x);
    if (cansee(tp->t_pos.y, tp->t_pos.x))
      if (!monster_is_invisible(tp) || player_has_true_sight())
        addcch(tp->t_disguise);
      else
        addcch(chat(tp->t_pos.y, tp->t_pos.x));
    else if (on(player, SEEMONST))
      addcch(tp->t_type | A_STANDOUT);
  }
  msg("Everything looks SO boring now.");
}

bool player_is_hasted(void)    { return player.t_flags & ISHASTE; }
void player_set_hasted(bool permanent)
{
  if (player_is_hasted())
  {
    no_command += rnd(8);
    player_stop_running();
    player_remove_hasted();
    daemon_extinguish_fuse(player_remove_hasted);
    msg("you faint from exhaustion");
  }
  else
  {
    player.t_flags |= ISHASTE;
    if (!permanent)
      daemon_start_fuse(player_remove_hasted, 0, HASTEDURATION, AFTER);
    msg("you feel yourself moving much faster");
  }

}
void player_remove_hasted(void)
{
  if (!player_is_hasted())
    return;

  player.t_flags &= ~ISHASTE;
  msg("you feel yourself slowing down");
}

bool player_is_running(void)    { return player.t_flags & ISRUN; }
void player_start_running(void) { player.t_flags |= ISRUN; }
void player_stop_running(void)  { player.t_flags &= ~ISRUN; }

bool player_is_blind(void) { return player.t_flags & ISBLIND; }
void player_set_blind(bool permanent)
{
  if (player_is_blind())
    daemon_lengthen_fuse(player_remove_blind, SEEDURATION);
  else
  {
    player.t_flags |= ISBLIND;
    look(false);
    if (!permanent)
      daemon_start_fuse(player_remove_blind, 0, SEEDURATION, AFTER);
    msg(player_is_hallucinating()
      ? "oh, bummer!  Everything is dark!  Help!"
      : "a cloak of darkness falls around you");
  }
}
void player_remove_blind(void)
{
  if (!player_is_blind())
    return;

  daemon_extinguish_fuse(player_remove_blind);
  player.t_flags &= ~ISBLIND;
  if (!(player_get_room()->r_flags & ISGONE))
    room_enter(player_get_pos());
  msg(player_is_hallucinating()
    ? "far out!  Everything is all cosmic again"
    : "the veil of darkness lifts");
}

bool player_is_levitating(void) { return player.t_flags & ISLEVIT; }
void player_start_levitating(bool permanent)
{
  if (player_is_levitating())
    daemon_lengthen_fuse(player_stop_levitating, LEVITDUR);
  else
  {
    player.t_flags |= ISLEVIT;
    look(false);
    if (!permanent)
      daemon_start_fuse(player_stop_levitating, 0, LEVITDUR, AFTER);
    msg(player_is_hallucinating()
      ? "oh, wow!  You're floating in the air!"
      : "you start to float in the air");
  }
}
void player_stop_levitating(void)
{
  if (!player_is_levitating())
    return;
  player.t_flags &= ~ISLEVIT;
  msg(player_is_hallucinating()
    ? "bummer!  You've hit the ground"
    : "you float gently to the ground");
}

bool player_has_confusing_attack(void)    { return player.t_flags & CANHUH; }
void player_set_confusing_attack(void)
{
  player.t_flags |= CANHUH;
  msg("your hands begin to glow %s", pick_color("red"));
}
void player_remove_confusing_attack(void) { player.t_flags &= ~CANHUH; }

coord * player_get_pos(void) { return &player.t_pos; }
void
player_set_pos(coord *new_pos)
{
  player.t_pos.x = new_pos->x;
  player.t_pos.y = new_pos->y;
}

struct room * player_get_room(void)         { return player.t_room; }
void player_set_room(struct room *new_room) { player.t_room = new_room; }

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

void
player_lose_health(int amount)
{
  player.t_stats.s_hpt -= amount;
}

int
player_get_armor(void)
{
  int ac = 0;
  THING const * const arm = pack_equipped_item(EQUIPMENT_ARMOR);
  THING const * const weapon = pack_equipped_item(EQUIPMENT_RHAND);
  int i;

  ac = arm ? arm->o_arm : player.t_stats.s_arm;
  if (weapon)
    ac -= weapon->o_arm;

  for (i = 0; i < RING_SLOTS_SIZE; ++i)
  {
    THING *ring = pack_equipped_item(ring_slots[i]);
    if (ring != NULL && ring->o_which == R_PROTECT)
      ac -= ring->o_arm;
  }

  return 20 - ac;
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
