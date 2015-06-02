#include <stdbool.h>

#include "pack.h"
#include "rings.h"
#include "misc.h"
#include "io.h"
#include "daemons.h"
#include "list.h"
#include "armor.h"
#include "level.h"
#include "rooms.h"
#include "monster.h"
#include "command.h"
#include "weapons.h"
#include "traps.h"
#include "os.h"
#include "state.h"
#include "rogue.h"

#include "player.h"

static THING player;
static int player_speed = 0;

static const int player_min_strength = 3;
static const int player_max_strength = 31;

int food_left;      /* Amount of food in hero's stomach */
int hungry_state = 0;/* How hungry is he */
int no_food = 0;    /* Number of levels without food */
int no_command = 0; /* Number of turns asleep */
int no_move = 0;    /* Number of turns held in place */

/* Duration of effects */
#define HUHDURATION     spread(20)  /* Confusion */
#define MFINDDURATION   spread(20)  /* Monster find */
#define HASTEDURATION   rnd(4)+4    /* Haste */
#define SEEDURATION     spread(850) /* See invisible / blind / hallucinating */
#define LEVITDUR        spread(30)  /* Levitation */
#define SLEEPTIME       spread(7)   /* Sleep */
#define STUCKTIME       spread(3)   /* Stuck */

/* The maximum for the player */
struct stats max_stats = { 16, 0, 1, 10, 12, {{1,4}}, 12 };

void* __player_ptr(void) { return &player; }

static int e_levels[] = {
  10L,
  20L,
  40L,
  80L,
  160L,
  320L,
  640L,
  1300L,
  2600L,
  5200L,
  13000L,
  26000L,
  50000L,
  100000L,
  200000L,
  400000L,
  800000L,
  2000000L,
  4000000L,
  8000000L,
  0L
};


static int
player_get_strength_bonuses(void)
{
  int bonuses = 0;
  for (int i = 0; i < RING_SLOTS_SIZE; ++i)
  {
    THING const* ring = pack_equipped_item(ring_slots[i]);
    if (ring != NULL && ring->o_which == R_ADDSTR)
      bonuses += ring->o_arm;
  }
  return bonuses;
}

static void
player_update_max_strength(void)
{
  int bonuses = player_get_strength_bonuses();
  if (player.t_stats.s_str - bonuses > max_stats.s_str)
    max_stats.s_str = player.t_stats.s_str - bonuses;
}

void
player_init(void)
{
  THING* obj;

  player.t_stats = max_stats;
  food_left = HUNGERTIME;

  /* Give him some food */
  pack_add(new_food(-1), true);

  /* And his suit of armor */
  obj = armor_create(RING_MAIL, false);
  obj->o_flags |= ISKNOW;
  obj->o_arm -= 1;
  pack_equip_item(obj);

  /* Give him his weaponry.  First a mace. */
  obj = weapon_create(MACE, false);
  obj->o_hplus  = 1;
  obj->o_dplus  = 1;
  obj->o_flags |= ISKNOW;
  pack_equip_item(obj);

  /* Now a +1 bow */
  obj = weapon_create(BOW, false);
  obj->o_hplus  = 1;
  obj->o_flags |= ISKNOW;
  pack_add(obj, true);
  set_last_weapon(obj);

  /* Now some arrows */
  obj = weapon_create(ARROW, false);
  obj->o_count  = rnd(15) + 25;
  obj->o_flags |= ISKNOW;
  pack_add(obj, true);
}

bool
player_load_state(void)
{
  return state_load_thing(&player)
    || state_load_int32(&player_speed);
}

bool
player_save_state(void)
{
  return state_save_thing(&player)
    || state_save_int32(player_speed);
}

bool
is_player(THING const* thing)
{
  return thing == &player;
}

int
player_save_throw(int which)
{
  if (which == VS_MAGIC)
    for (int i = 0; i < RING_SLOTS_SIZE; ++i)
    {
      THING *ring = pack_equipped_item(ring_slots[i]);
      if (ring != NULL && ring->o_which == R_PROTECT)
        which -= ring->o_arm;
    }

  int need = 14 + which - player.t_stats.s_lvl / 2;
  return (roll(1, 20) >= need);
}

bool player_has_true_sight(void) { return player.t_flags & CANSEE; }

void
player_add_true_sight(bool permanent)
{
  if (player_has_true_sight())
    daemon_lengthen_fuse(player_remove_true_sight, SEEDURATION);
  else
  {
    player.t_flags |= CANSEE;
    look(false);
    if (!permanent)
      daemon_start_fuse(player_remove_true_sight, 0, SEEDURATION, AFTER);
  }
}
void
player_remove_true_sight(void)
{
  /* Hide all invisible monsters */
  for (THING* monster = mlist; monster != NULL; monster = monster->l_next)
    if (monster_is_invisible(monster) && see_monst(monster))
      mvaddcch(monster->t_pos.y, monster->t_pos.x, monster->t_oldch);

  /* Set flag */
  player.t_flags &= ~CANSEE;
}

bool player_is_confused(void) { return player.t_flags & ISHUH; }

void
player_set_confused(bool permanent)
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
  msg("wait, what's going on here. Huh? What? Who?");
}

void
player_remove_confused(void)
{
  player.t_flags &= ~ISHUH;
  msg("you feel less confused now");
}

bool player_is_held(void)     { return player.t_flags & ISHELD; }
void player_set_held(void)    { player.t_flags |= ISHELD; }
void player_remove_held(void) { player.t_flags &= ~ISHELD; }

bool player_can_sense_monsters(void)    { return player.t_flags & SEEMONST; }

void
player_add_sense_monsters(bool permanent)
{
  if (!permanent)
      daemon_start_fuse(player_remove_sense_monsters, 0, MFINDDURATION, AFTER);

  player.t_flags |= SEEMONST;

  bool spotted_something = false;
  for (THING* monster = mlist; monster != NULL; monster = monster->l_next)
    if (!see_monst(monster))
    {
      mvaddcch(monster->t_pos.y, monster->t_pos.x,
          (player_is_hallucinating() ? (char)(rnd(26) + 'A') : monster->t_type)
            | A_STANDOUT);
      spotted_something = true;
    }

  if (!spotted_something)
    msg("you have a strange feeling for a moment, then it passes");
}

void
player_remove_sense_monsters(void)
{
  player.t_flags &= ~SEEMONST;

  for (THING* monster = mlist; monster != NULL; monster = monster->l_next)
    if (!see_monst(monster))
      mvaddcch(monster->t_pos.y, monster->t_pos.x, monster->t_oldch);
}

bool player_is_hallucinating(void)     { return player.t_flags & ISHALU; }

void
player_set_hallucinating(bool permanent)
{
  if (player_is_hallucinating())
    daemon_lengthen_fuse(player_remove_hallucinating, SEEDURATION);
  else
  {
    if (player_can_sense_monsters())
      player_add_sense_monsters(true);
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
  if (!player_is_hallucinating())
    return;

  daemon_kill(daemon_change_visuals);
  player.t_flags &= ~ISHALU;

  if (player_is_blind())
    return;

  /* undo the things */
  for (THING* tp = lvl_obj; tp != NULL; tp = tp->l_next)
    if (cansee(tp->o_pos.y, tp->o_pos.x))
      mvaddcch(tp->o_pos.y, tp->o_pos.x, (chtype)tp->o_type);

  /* undo the monsters */
  for (THING* tp = mlist; tp != NULL; tp = tp->l_next)
  {
    if (cansee(tp->t_pos.y, tp->t_pos.x))
      if (!monster_is_invisible(tp) || player_has_true_sight())
        mvaddcch(tp->t_pos.y, tp->t_pos.x, tp->t_disguise);
      else
        mvaddcch(tp->t_pos.y, tp->t_pos.x, level_get_ch(tp->t_pos.y, tp->t_pos.x));
    else if (player_can_sense_monsters())
      mvaddcch(tp->t_pos.y, tp->t_pos.x, tp->t_type | A_STANDOUT);
  }
  msg("Everything feel your senses returning to normal");
}

int player_get_speed(void)    { return player_speed; }

void
player_increase_speed(bool permanent)
{
  player_speed++;
  if (!permanent)
    daemon_start_fuse(player_decrease_speed, 1, HASTEDURATION, AFTER);
  msg("you feel yourself moving much faster");
}

void
player_decrease_speed(void)
{
  player_speed--;
  msg("you feel yourself slowing down");
}

bool player_is_running(void)    { return player.t_flags & ISRUN; }
void player_start_running(void) { player.t_flags |= ISRUN; }
void player_stop_running(void)  { player.t_flags &= ~ISRUN; }

bool player_is_blind(void) { return player.t_flags & ISBLIND; }

void
player_set_blind(bool permanent)
{
  if (player_is_blind())
    daemon_lengthen_fuse(player_remove_blind, SEEDURATION);
  else
  {
    player.t_flags |= ISBLIND;
    look(false);
    if (!permanent)
      daemon_start_fuse(player_remove_blind, 0, SEEDURATION, AFTER);
    msg("a cloak of darkness falls around you");
  }
}

void
player_remove_blind(void)
{
  if (!player_is_blind())
    return;

  daemon_extinguish_fuse(player_remove_blind);
  player.t_flags &= ~ISBLIND;
  if (!(player_get_room()->r_flags & ISGONE))
    room_enter(player_get_pos());
  msg("the veil of darkness lifts");
}

bool player_is_levitating(void) { return player.t_flags & ISLEVIT; }

void
player_start_levitating(bool permanent)
{
  if (player_is_levitating())
    daemon_lengthen_fuse(player_stop_levitating, LEVITDUR);
  else
  {
    player.t_flags |= ISLEVIT;
    look(false);
    if (!permanent)
      daemon_start_fuse(player_stop_levitating, 0, LEVITDUR, AFTER);
    msg("you start to float in the air");
  }
}
void player_stop_levitating(void)
{
  if (!player_is_levitating())
    return;
  player.t_flags &= ~ISLEVIT;
  msg("you float gently to the ground");
}

bool player_has_confusing_attack(void)    { return player.t_flags & CANHUH; }

void
player_set_confusing_attack(void)
{
  player.t_flags |= CANHUH;
  msg("your hands begin to glow %s", pick_color("red"));
}

void player_remove_confusing_attack(void) { player.t_flags &= ~CANHUH; }

void
player_fall_asleep(void)
{
  no_command += SLEEPTIME;
  player_stop_running();
  msg("you fall asleep");
}

void player_become_stuck(void)
{
  no_move += STUCKTIME;
  player_stop_running();
}

void player_become_poisoned(void)
{
  if (player_has_ring_with_ability(R_SUSTSTR))
    msg("you feel momentarily nauseous");
  else
  {
    player_modify_strength(-(rnd(3) + 1));
    msg("you feel very sick now");
    player_remove_hallucinating();
  }
}

bool
player_is_stealthy(void)
{
  return player_has_ring_with_ability(R_STEALTH)
    || player_is_levitating();
}

void player_teleport(coord *target)
{
  coord new_pos;
  coord const* player_pos = player_get_pos();

  /* Set target location */
  if (target == NULL)
    do
      room_find_floor(NULL, &new_pos, false, true);
    while (same_coords(&new_pos, player_pos));
  else
  {
    new_pos.y = target->y;
    new_pos.x = target->x;
  }

  /* Move target */
  mvaddcch(player.t_pos.y, player.t_pos.x, floor_at());
  if (roomin(&new_pos) != player_get_room())
  {
    room_leave(player_get_pos());
    player_set_pos(&new_pos);
    room_enter(player_get_pos());
  }
  else
  {
    player_set_pos(&new_pos);
    look(true);
  }

  /* Print @ new location */
  mvaddcch(new_pos.y, new_pos.x, PLAYER);
  if (player_is_held())
  {
    player_remove_held();
    vf_hit = 0;
  }
  no_move = 0;
  command_stop(true);
  flushinp();
  msg("suddenly you're somewhere else");
}

bool
player_search(void)
{
  int probinc = (player_is_hallucinating() ? 3:0) + (player_is_blind() ? 2:0);
  bool found = false;
  coord *player_pos = player_get_pos();

  for (int y = player_pos->y - 1; y <= player_pos->y + 1; y++)
    for (int x = player_pos->x - 1; x <= player_pos->x + 1; x++)
    {
      /* Real wall/floor/shadow */
      int flags = level_get_flags(y, x);
      if (flags & F_REAL)
        continue;

      char chatyx = level_get_ch(y, x);
      switch (chatyx)
      {
        case VWALL: case HWALL:
          if (!rnd(5 + probinc))
          {
            level_set_ch(y, x, DOOR);
            msg("a secret door");
            found = true;
            flags |= F_REAL;
            level_set_flags(y, x, (char)flags);
          }
          break;

        case FLOOR:
          if (!rnd(2 + probinc))
          {
            level_set_ch(y, x, TRAP);

            if (player_is_hallucinating())
              msg(trap_names[rnd(NTRAPS)]);
            else {
              msg(trap_names[flags & F_TMASK]);
              flags |= F_SEEN;
              level_set_flags(y, x, (char)flags);
            }

            found = true;
            flags |= F_SEEN;
            level_set_flags(y, x, (char)flags);
          }
          break;

        case SHADOW:
          if (!rnd(3 + probinc))
          {
            level_set_ch(y, x, PASSAGE);
            found = true;
            flags |= F_REAL;
            level_set_flags(y, x, (char)flags);
          }
          break;
      }
    }

  if (found)
  {
    look(false);
    running = false;
  }

  return true;
}

coord* player_get_pos(void) { return &player.t_pos; }
int player_y(void)          { return player.t_pos.y; }
int player_x(void)          { return player.t_pos.x; }

void
player_set_pos(coord* new_pos)
{
  player.t_pos.x = new_pos->x;
  player.t_pos.y = new_pos->y;
}

struct room* player_get_room(void)          { return player.t_room; }
void player_set_room(struct room* new_room) { player.t_room = new_room; }
int player_get_strength(void)               { return player.t_stats.s_str; }

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
  if (player.t_stats.s_str < player_min_strength)
    player.t_stats.s_str = player_min_strength;
  else if (player.t_stats.s_str > player_max_strength)
    player.t_stats.s_str = player_max_strength;

  player_update_max_strength();
}

int player_get_health(void)     { return player.t_stats.s_hpt; }
int player_get_max_health(void) { return player.t_stats.s_maxhp; }

void
player_restore_health(int amount, bool can_raise_total)
{
  player.t_stats.s_hpt += amount;

  if (can_raise_total)
  {
    int extra_max_hp = 0;
    if (player_get_health() > player_get_max_health() + player_get_level() + 1)
      ++extra_max_hp;
    if (player_get_health() > player_get_max_health())
      ++extra_max_hp;
    if (amount > 0)
      player_modify_max_health(extra_max_hp);
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
  THING const * const arm = pack_equipped_item(EQUIPMENT_ARMOR);
  THING const * const weapon = pack_equipped_item(EQUIPMENT_RHAND);

  int ac = arm ? arm->o_arm : player.t_stats.s_arm;
  if (weapon)
    ac -= weapon->o_arm;

  for (int i = 0; i < RING_SLOTS_SIZE; ++i)
  {
    THING const* ring = pack_equipped_item(ring_slots[i]);
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
  int next_level = e_levels[player.t_stats.s_lvl -1] + 1L;
  if (next_level < player.t_stats.s_exp)
    return;

  player.t_stats.s_exp = next_level;
  player_check_for_level_up();
  msg("you suddenly feel much more skillful");
}

void
player_check_for_level_up(void)
{
  int i;
  int old_level = player.t_stats.s_lvl;

  for (i = 0; e_levels[i] != 0; ++i)
    if (e_levels[i] > player.t_stats.s_exp)
      break;

  ++i;
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
