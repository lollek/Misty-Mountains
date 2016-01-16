#include "Coordinate.h"
#include "pack.h"
#include "rings.h"
#include "misc.h"
#include "io.h"
#include "daemons.h"
#include "armor.h"
#include "level.h"
#include "rooms.h"
#include "monster.h"
#include "command.h"
#include "weapons.h"
#include "traps.h"
#include "os.h"
#include "rogue.h"
#include "colors.h"

#include "player.h"

Player::Player() {
  this->t_stats = player_max_stats;

  /* Give him some food */
  pack_add(new_food(-1), true);

  /* And his suit of armor */
  Item* armor = armor_create(RING_MAIL, false);
  armor->o_flags |= ISKNOW;
  armor->o_arm -= 1;
  pack_equip_item(armor);

  /* Give him his weaponry.  First a mace. */
  Item* mace = weapon_create(MACE, false);
  mace->o_hplus  = 1;
  mace->o_dplus  = 1;
  mace->o_flags |= ISKNOW;
  pack_equip_item(mace);

  /* Now a +1 bow */
  Item* bow = weapon_create(BOW, false);
  bow->o_hplus  = 1;
  bow->o_flags |= ISKNOW;
  pack_add(bow, true);
  weapon_set_last_used(bow);

  /* Now some arrows */
  Item* arrow = weapon_create(ARROW, false);
  arrow->o_count  = os_rand_range(15) + 25;
  arrow->o_flags |= ISKNOW;
  pack_add(arrow, true);
}

int Player::get_exp() const {
  return this->t_stats.s_exp;
}

void Player::earn_exp(int amount) {
  this->t_stats.s_exp += amount;
}

Player* player;

static const int player_min_strength = 3;
static const int player_max_strength = 31;

int          player_turns_without_action = 0;
int          player_turns_without_moving = 0;
bool         player_alerted              = false;
struct stats player_max_stats = { 16, 0, 1, 10, 12, {{1,4}}, 12 };

/* Duration of effects */
#define HUHDURATION     spread(20)  /* Confusion */
#define MFINDDURATION   spread(20)  /* Monster find */
#define HASTEDURATION   os_rand_range(4)+4    /* Haste */
#define SEEDURATION     spread(850) /* See invisible / blind / hallucinating */
#define LEVITDUR        spread(30)  /* Levitation */
#define SLEEPTIME       spread(7)   /* Sleep */
#define STUCKTIME       spread(3)   /* Stuck */


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
player_get_strength_bonuses()
{
  int bonuses = 0;
  for (int i = 0; i < PACK_RING_SLOTS; ++i)
  {
    Item const* ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring != nullptr && ring->o_which == R_ADDSTR)
      bonuses += ring->o_arm;
  }
  return bonuses;
}

static void
player_update_max_strength()
{
  int bonuses = player_get_strength_bonuses();
  if (player->t_stats.s_str - bonuses > player_max_stats.s_str)
    player_max_stats.s_str = player->t_stats.s_str - bonuses;
}

int
player_save_throw(int which)
{
  if (which == VS_MAGIC)
    for (int i = 0; i < PACK_RING_SLOTS; ++i)
    {
      Item* ring = pack_equipped_item(pack_ring_slots[i]);
      if (ring != nullptr && ring->o_which == R_PROTECT)
        which -= ring->o_arm;
    }

  int need = 14 + which - player->t_stats.s_lvl / 2;
  return (roll(1, 20) >= need);
}

bool player_has_true_sight() { return player->t_flags & CANSEE; }

void
player_add_true_sight(bool permanent)
{
  if (player_has_true_sight())
    daemon_lengthen_fuse(player_remove_true_sight, SEEDURATION);
  else
  {
    player->t_flags |= CANSEE;
    look(false);
    if (!permanent)
      daemon_start_fuse(player_remove_true_sight, 0, SEEDURATION, AFTER);
  }
}
void
player_remove_true_sight(__attribute__((unused)) int)
{
  monster_hide_all_invisible();
  player->t_flags &= ~CANSEE;
}

bool player_is_confused() { return player->t_flags & ISHUH; }

void
player_set_confused(bool permanent)
{
  if (player_is_confused())
    daemon_lengthen_fuse(player_remove_confused, HUHDURATION);
  else
  {
    player->t_flags |= ISHUH;
    look(false);
    if (!permanent)
      daemon_start_fuse(player_remove_confused, 0, HUHDURATION, AFTER);
  }
  io_msg("wait, what's going on here. Huh? What? Who?");
}

void
player_remove_confused(__attribute__((unused)) int)
{
  player->t_flags &= ~ISHUH;
  io_msg("you feel less confused now");
}

bool player_is_held()     { return player->t_flags & ISHELD; }
void player_set_held()    { player->t_flags |= ISHELD; }
void player_remove_held() { player->t_flags &= ~ISHELD; }

bool player_can_sense_monsters()    { return player->t_flags & SEEMONST; }

void
player_add_sense_monsters(bool permanent)
{
  if (!permanent)
      daemon_start_fuse(player_remove_sense_monsters, 0, MFINDDURATION, AFTER);

  player->t_flags |= SEEMONST;

  bool spotted_something = monster_sense_all_hidden();
  if (!spotted_something)
    io_msg("you have a strange feeling for a moment, then it passes");
}

void
player_remove_sense_monsters(__attribute__((unused)) int)
{
  player->t_flags &= ~SEEMONST;
  monster_unsense_all_hidden();
}

bool player_is_hallucinating()     { return player->t_flags & ISHALU; }

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
    player->t_flags |= ISHALU;
    look(false);
    if (!permanent)
      daemon_start_fuse(player_remove_hallucinating, 0, SEEDURATION, AFTER);
    io_msg("Oh, wow!  Everything seems so cosmic!");
  }
}

void player_remove_hallucinating(__attribute__((unused)) int)
{
  if (!player_is_hallucinating()) {
    return;
  }

  daemon_kill(daemon_change_visuals);
  player->t_flags &= ~ISHALU;

  if (player_is_blind()) {
    return;
  }

  /* undo the things */
  for (Item* tp : level_items) {
    if (cansee(tp->get_y(), tp->get_x())) {
      mvaddcch(tp->get_y(), tp->get_x(), static_cast<chtype>(tp->o_type));
    }
  }

  /* undo the monsters */
  monster_print_all();
  io_msg("You feel your senses returning to normal");
}

int player_get_speed()    { return player->speed; }

void
player_increase_speed(bool permanent)
{
  player->speed++;
  if (!permanent)
    daemon_start_fuse(player_decrease_speed, 1, HASTEDURATION, AFTER);
  io_msg("you feel yourself moving much faster");
}

void
player_decrease_speed(__attribute__((unused)) int)
{
  player->speed--;
  io_msg("you feel yourself slowing down");
}

bool player_is_running()    { return player->t_flags & ISRUN; }
void player_start_running() { player->t_flags |= ISRUN; }
void player_stop_running()  { player->t_flags &= ~ISRUN; }

bool player_is_blind() { return player->t_flags & ISBLIND; }

void
player_set_blind(bool permanent)
{
  if (player_is_blind())
    daemon_lengthen_fuse(player_remove_blind, SEEDURATION);
  else
  {
    player->t_flags |= ISBLIND;
    look(false);
    if (!permanent)
      daemon_start_fuse(player_remove_blind, 0, SEEDURATION, AFTER);
    io_msg("a cloak of darkness falls around you");
  }
}

void
player_remove_blind(__attribute__((unused)) int)
{
  if (!player_is_blind())
    return;

  daemon_extinguish_fuse(player_remove_blind);
  player->t_flags &= ~ISBLIND;
  if (!(player_get_room()->r_flags & ISGONE))
    room_enter(player_get_pos());
  io_msg("the veil of darkness lifts");
}

bool player_is_levitating() { return player->t_flags & ISLEVIT; }

void
player_start_levitating(bool permanent)
{
  if (player_is_levitating())
    daemon_lengthen_fuse(player_stop_levitating, LEVITDUR);
  else
  {
    player->t_flags |= ISLEVIT;
    look(false);
    if (!permanent)
      daemon_start_fuse(player_stop_levitating, 0, LEVITDUR, AFTER);
    io_msg("you start to float in the air");
  }
}
void player_stop_levitating(__attribute__((unused)) int)
{
  if (!player_is_levitating())
    return;
  player->t_flags &= ~ISLEVIT;
  io_msg("you float gently to the ground");
}

bool player_has_confusing_attack()    { return player->t_flags & CANHUH; }

void
player_set_confusing_attack()
{
  player->t_flags |= CANHUH;
  io_msg("your hands begin to glow %s",
         player_is_hallucinating() ? color_random().c_str() : "red");
}

void player_remove_confusing_attack() { player->t_flags &= ~CANHUH; }

void
player_fall_asleep()
{
  player_turns_without_action += SLEEPTIME;
  player_stop_running();
  io_msg("you fall asleep");
}

void player_become_stuck()
{
  player_turns_without_moving += STUCKTIME;
  player_stop_running();
}

void player_become_poisoned()
{
  if (player_has_ring_with_ability(R_SUSTSTR))
    io_msg("you feel momentarily nauseous");
  else
  {
    player_modify_strength(-(os_rand_range(3) + 1));
    io_msg("you feel very sick now");
    player_remove_hallucinating(0);
  }
}

bool
player_is_stealthy()
{
  return player_has_ring_with_ability(R_STEALTH)
    || player_is_levitating();
}

void player_teleport(Coordinate *target)
{
  Coordinate new_pos;
  Coordinate const* player_pos = player_get_pos();

  /* Set target location */
  if (target == nullptr)
    do
      room_find_floor(nullptr, &new_pos, false, true);
    while (new_pos == *player_pos);
  else
  {
    new_pos.y = target->y;
    new_pos.x = target->x;
  }

  /* Move target */
  mvaddcch(player->t_pos.y, player->t_pos.x, static_cast<chtype>(floor_at()));
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
    monster_flytrap_hit = 0;
  }
  player_turns_without_moving = 0;
  command_stop(true);
  flushinp();
  io_msg("suddenly you're somewhere else");
}

bool
player_search()
{
  int probinc = (player_is_hallucinating() ? 3:0) + (player_is_blind() ? 2:0);
  bool found = false;
  Coordinate *player_pos = player_get_pos();

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
          if (!os_rand_range(5 + probinc))
          {
            level_set_ch(y, x, DOOR);
            io_msg("a secret door");
            found = true;
            flags |= F_REAL;
            level_set_flags(y, x, static_cast<char>(flags));
          }
          break;

        case FLOOR:
          if (!os_rand_range(2 + probinc))
          {
            level_set_ch(y, x, TRAP);

            if (player_is_hallucinating())
              io_msg(trap_names[os_rand_range(NTRAPS)].c_str());
            else {
              io_msg(trap_names[flags & F_TMASK].c_str());
              flags |= F_SEEN;
              level_set_flags(y, x, static_cast<char>(flags));
            }

            found = true;
            flags |= F_SEEN;
            level_set_flags(y, x, static_cast<char>(flags));
          }
          break;

        case SHADOW:
          if (!os_rand_range(3 + probinc))
          {
            level_set_ch(y, x, PASSAGE);
            found = true;
            flags |= F_REAL;
            level_set_flags(y, x, static_cast<char>(flags));
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

Coordinate* player_get_pos() { return &player->t_pos; }
int player_y()          { return player->t_pos.y; }
int player_x()          { return player->t_pos.x; }

void
player_set_pos(Coordinate* new_pos)
{
  player->t_pos.x = new_pos->x;
  player->t_pos.y = new_pos->y;
}

struct room* player_get_room()          { return player->t_room; }
void player_set_room(struct room* new_room) { player->t_room = new_room; }
int player_get_strength()               { return player->t_stats.s_str; }

bool
player_strength_is_weakened()
{
  return player->t_stats.s_str < player_max_stats.s_str;
}

void
player_restore_strength()
{
  player->t_stats.s_str = player_max_stats.s_str + player_get_strength_bonuses();
}

void
player_modify_strength(int amount)
{
  player->t_stats.s_str += amount;
  if (player->t_stats.s_str < player_min_strength)
    player->t_stats.s_str = player_min_strength;
  else if (player->t_stats.s_str > player_max_strength)
    player->t_stats.s_str = player_max_strength;

  player_update_max_strength();
}

int player_get_health()     { return player->t_stats.s_hpt; }
int player_get_max_health() { return player->t_stats.s_maxhp; }

void
player_restore_health(int amount, bool can_raise_total)
{
  player->t_stats.s_hpt += amount;

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
    player->t_stats.s_hpt = player_get_max_health();
}

bool
player_is_hurt()
{
  return player_get_health() != player_get_max_health();
}

void
player_modify_max_health(int amount)
{
  player->t_stats.s_maxhp += amount;
  if (player->t_stats.s_hpt > player->t_stats.s_maxhp)
    player->t_stats.s_hpt = player->t_stats.s_maxhp;
}

void
player_lose_health(int amount)
{
  player->t_stats.s_hpt -= amount;
}

int
player_get_armor()
{
  Item const* const arm = pack_equipped_item(EQUIPMENT_ARMOR);
  Item const* const weapon = pack_equipped_item(EQUIPMENT_RHAND);

  int ac = arm ? arm->o_arm : player->t_stats.s_arm;
  if (weapon)
    ac -= weapon->o_arm;

  for (int i = 0; i < PACK_RING_SLOTS; ++i)
  {
    Item const* ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring != nullptr && ring->o_which == R_PROTECT)
      ac -= ring->o_arm;
  }

  return 20 - ac;
}

int
player_get_level()
{
  return player->t_stats.s_lvl;
}

void
player_raise_level()
{
  int next_level = e_levels[player->t_stats.s_lvl -1] + 1L;
  if (next_level < player->t_stats.s_exp)
    return;

  player->t_stats.s_exp = next_level;
  player_check_for_level_up();
  io_msg("you suddenly feel much more skillful");
}

void
player_check_for_level_up()
{
  int i;
  int old_level = player->t_stats.s_lvl;

  for (i = 0; e_levels[i] != 0; ++i)
    if (e_levels[i] > player->t_stats.s_exp)
      break;

  ++i;
  player->t_stats.s_lvl = i;

  if (i > old_level)
  {
    int add_to_hp = roll(i - old_level, 10);
    player_modify_max_health(add_to_hp);
    player_restore_health(add_to_hp, false);
    io_msg("welcome to level %d", player->t_stats.s_lvl);
  }
}

void
player_lower_level()
{
  --player->t_stats.s_lvl;
  if (player->t_stats.s_lvl == 0)
  {
    player->t_stats.s_exp = 0;
    player->t_stats.s_lvl = 1;
  }
  else
    player->t_stats.s_exp = e_levels[player->t_stats.s_lvl-1] +1L;
}



bool
player_has_ring_with_ability(int ability)
{
  int i;
  for (i = 0; i < PACK_RING_SLOTS; ++i)
  {
    Item* ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring != nullptr && ring->o_which == ability)
      return true;
  }
  return false;
}

