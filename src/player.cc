#include <string>

#include "error_handling.h"
#include "game.h"
#include "Coordinate.h"
#include "pack.h"
#include "rings.h"
#include "misc.h"
#include "io.h"
#include "daemons.h"
#include "armor.h"
#include "level.h"
#include "level_rooms.h"
#include "monster.h"
#include "command.h"
#include "weapons.h"
#include "traps.h"
#include "os.h"
#include "rogue.h"
#include "colors.h"

#include "player.h"

using namespace std;

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



Player::Player() :
  Character(player_max_stats.s_str, player_max_stats.s_exp, player_max_stats.s_lvl,
            player_max_stats.s_arm, player_max_stats.s_hpt, player_max_stats.s_dmg,
            Coordinate(), nullptr, 0, '@')  {

  /* Give him some food */
  pack_add(new_food(-1), true);

  /* And his suit of armor */
  Item* armor = new Armor(Armor::Type::RING_MAIL, false);
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

int Player::get_armor() const {
  // If we are naked, use base armor, otherwise use armor's
  Item const* const arm = pack_equipped_item(EQUIPMENT_ARMOR);
  int ac = arm ? arm->o_arm : Character::get_armor();

  // If weapon help protection, add it
  Item const* const weapon = pack_equipped_item(EQUIPMENT_RHAND);
  if (weapon)
    ac -= weapon->o_arm;

  // If rings help, add their stats as well
  for (int i = 0; i < PACK_RING_SLOTS; ++i)
  {
    Item const* ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring != nullptr && ring->o_which == Ring::Type::PROTECT)
      ac -= ring->o_arm;
  }

  return 20 - ac;
}

bool Player::has_seen_stairs() const {

  // It is on the map
  if (Game::level->is_discovered(Game::level->get_stairs_pos()))
    return true;

  // It's under the player
  if (get_position() == Game::level->get_stairs_pos())
    return true;

  // if a monster is on the stairs, this gets hairy
  Monster* monster = Game::level->get_monster(Game::level->get_stairs_pos());
  if (monster != nullptr) {

    // if it's visible and awake, it must have moved there
    if (monster_seen_by_player(monster) && monster->is_chasing())
      return true;

    if (can_sense_monsters()             // if she can detect monster
        && monster->t_oldch == STAIRS)   // and there once were stairs
      return true;                       // it must have moved there
  }

  return false;
}

bool Player::can_see(Coordinate const& coord) const {

  Coordinate const& player_pos = get_position();

  if (is_blind()) {
    return false;
  }

  if (dist(coord.y, coord.x, player_pos.y, player_pos.x) < LAMPDIST) {
    if (Game::level->is_passage(coord)) {
      if (coord.y != player_pos.y && coord.x != player_pos.x &&
          !step_ok(Game::level->get_ch(player_pos.x, coord.y))
          && !step_ok(Game::level->get_ch(coord.x, player_pos.y))) {
        return false;
      }
    }
    return true;
  }

  /* We can only see if the hero in the same room as
   * the coordinate and the room is lit or if it is close.  */
  struct room const* rer = Game::level->get_room(coord);
  if (rer != get_room())
    return false;

  return !(rer->r_flags & ISDARK);
}

string Player::get_attack_string(bool successful_hit) const {
  vector<string> hit_string {
      "hit"
    , "scored an excellent hit on"
    , "have injured"
    , "swing and hit"
  };

  vector<string> miss_string {
      "miss"
    , "swing and miss"
    , "barely miss"
    , "don't hit"
  };

  size_t i = static_cast<size_t>(os_rand_range(4));
  if (successful_hit) {
    return hit_string.at(i);
  } else {
    return miss_string.at(i);
  }
}

string Player::get_name() const{
  return "you";
}

void Player::waste_time(int rounds) const {
  for (int i = 0; i < rounds; ++i) {
    daemon_run_before();
    daemon_run_after();
  }
}


int Player::get_strength_with_bonuses() const {
  int bonuses = 0;
  for (int i = 0; i < PACK_RING_SLOTS; ++i)
  {
    Item const* ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring != nullptr && ring->o_which == Ring::Type::ADDSTR)
      bonuses += ring->o_arm;
  }
  return get_strength() + bonuses;
}

bool Player::saving_throw(int which) const {
  if (which == VS_MAGIC)
    for (int i = 0; i < PACK_RING_SLOTS; ++i)
    {
      Item* ring = pack_equipped_item(pack_ring_slots[i]);
      if (ring != nullptr && ring->o_which == Ring::Type::PROTECT)
        which -= ring->o_arm;
    }

  int need = 14 + which - get_level() / 2;
  return (roll(1, 20) >= need);
}

bool Player::has_true_sight() const {
  return Character::has_true_sight() ||
    player->has_ring_with_ability(Ring::Type::SEEINVIS);
}

void Player::set_true_sight() {
  if (Character::has_true_sight()) {
    daemon_lengthen_fuse(daemon_function::remove_true_sight, SEEDURATION);

  } else {
    Character::set_true_sight();
    look(false);
    daemon_start_fuse(daemon_function::remove_true_sight, SEEDURATION, AFTER);
  }
}

void Player::remove_true_sight() {
  monster_hide_all_invisible();
  Character::remove_true_sight();
}

void Player::set_confused() {
  if (is_confused()) {
    daemon_lengthen_fuse(daemon_function::set_not_confused, HUHDURATION);

  } else {
    Character::set_confused();
    look(false);
    daemon_start_fuse(daemon_function::set_not_confused, HUHDURATION, AFTER);
  }
  io_msg("wait, what's going on here. Huh? What? Who?");
}

void Player::set_not_confused() {
  Character::set_not_confused();
  io_msg("you feel less confused now");
}

bool Player::can_sense_monsters() const {
  return senses_monsters;
}

void Player::set_sense_monsters() {
  daemon_start_fuse(daemon_function::remove_sense_monsters, MFINDDURATION, AFTER);

  senses_monsters = true;

  bool spotted_something = monster_sense_all_hidden();
  if (!spotted_something) {
    io_msg("you have a strange feeling for a moment, then it passes");
  }
}

void Player::remove_sense_monsters() {
  senses_monsters = false;
  monster_unsense_all_hidden();
}

void Player::set_hallucinating() {

  if (is_hallucinating()) {
    daemon_lengthen_fuse(daemon_function::set_not_hallucinating, SEEDURATION);

  } else {
    daemon_start(change_visuals, BEFORE);
    Character::set_hallucinating();
    look(false);
    daemon_start_fuse(daemon_function::set_not_hallucinating, SEEDURATION, AFTER);
    io_msg("Oh, wow!  Everything seems so cosmic!");
  }
}

void Player::set_not_hallucinating() {
  if (!is_hallucinating()) {
    return;
  }

  daemon_kill(change_visuals);
  Character::set_not_hallucinating();

  if (is_blind()) {
    return;
  }

  // Untrippify items on the level
  for (Item* tp : Game::level->items) {
    if (can_see(tp->get_pos())) {
      Game::io->print_color(tp->get_x(), tp->get_y(), tp->o_type);
    }
  }

  // Untrippify all monsters on the level
  monster_print_all();
  io_msg("You feel your senses returning to normal");
}

int Player::get_speed() const {
  return speed;
}

void Player::increase_speed() {
  speed++;
  daemon_start_fuse(daemon_function::decrease_speed, HASTEDURATION, AFTER);
  io_msg("you feel yourself moving much faster");
}

void Player::decrease_speed() {
  speed--;
  io_msg("you feel yourself slowing down");
}

void Player::set_blind() {

  if (is_blind()) {
    daemon_lengthen_fuse(daemon_function::set_not_blind, SEEDURATION);

  } else {
    Character::set_blind();
    look(false);
    daemon_start_fuse(daemon_function::set_not_blind, SEEDURATION, AFTER);
    io_msg("a cloak of darkness falls around you");
  }
}

void Player::set_not_blind() {

  if (!is_blind())
    return;

  daemon_extinguish_fuse(daemon_function::set_not_blind);
  Character::set_not_blind();
  if (!(get_room()->r_flags & ISGONE))
    room_enter(get_position());
  io_msg("the veil of darkness lifts");
}

void Player::set_levitating() {
  if (is_levitating()) {
    daemon_lengthen_fuse(daemon_function::set_not_levitating, LEVITDUR);

  } else {
    Character::set_levitating();
    look(false);
    daemon_start_fuse(daemon_function::set_not_levitating, LEVITDUR, AFTER);
    io_msg("you start to float in the air");
  }
}

void Player::set_not_levitating() {
  if (!is_levitating()) {
    return;
  }

  Character::set_not_levitating();
  io_msg("you float gently to the ground");
}

void Player::set_confusing_attack()
{
  Character::set_confusing_attack();
  io_msg("your hands begin to glow %s",
         is_hallucinating() ? color_random().c_str() : "red");
}

void Player::fall_asleep() {
  player_turns_without_action += SLEEPTIME;
  set_not_running();
  io_msg("you fall asleep");
}

void Player::become_stuck() {
  player_turns_without_moving += STUCKTIME;
  set_not_running();
}

void Player::become_poisoned() {
  if (player->has_ring_with_ability(Ring::Type::SUSTSTR)) {
    io_msg("you feel momentarily nauseous");

  } else {
    modify_strength(-(os_rand_range(3) + 1));
    io_msg("you feel very sick now");
    set_not_hallucinating();
  }
}

bool Player::is_stealthy() const {
  return player->has_ring_with_ability(Ring::Type::STEALTH)
    || is_levitating();
}

void Player::teleport(Coordinate const* target)
{
  Coordinate new_pos;
  Coordinate const& player_pos = get_position();

  // Set target location (nullptr means we generate a random position)
  if (target == nullptr) {
    do {
      Game::level->get_random_room_coord(nullptr, &new_pos, 0, true);
    } while (new_pos == player_pos);

  } else {
    new_pos.y = target->y;
    new_pos.x = target->x;
  }

  // Move target
  Game::io->print_color(get_position().x, get_position().y, floor_at());
  if (Game::level->get_room(new_pos) != get_room()) {
    room_leave(get_position());
    set_position(new_pos);
    room_enter(get_position());

  } else {
    set_position(new_pos);
    look(true);
  }

  /* Print @ new location */
  Game::io->print_color(new_pos.x, new_pos.y, get_type());
  if (is_held())
  {
    set_not_held();
    monster_flytrap_hit = 0;
  }
  player_turns_without_moving = 0;
  command_stop(true);
  flushinp();
  io_msg("suddenly you're somewhere else");
}

void Player::search() {
  int increased_difficulty = (is_hallucinating() ? 3 : 0) + (is_blind() ? 2 : 0);

  bool found = false;
  Coordinate const& player_pos = get_position();
  for (int y = player_pos.y - 1; y <= player_pos.y + 1; y++) {
    for (int x = player_pos.x - 1; x <= player_pos.x + 1; x++) {

      // If it's real, dont bother
      if (Game::level->is_real(x, y)) {
        continue;
      }

      // If fake, give player a chance to discover it
      switch (Game::level->get_ch(x, y)) {

        case VWALL: case HWALL:
          if (!os_rand_range(5 + increased_difficulty)) {
            Game::level->set_ch(x, y, DOOR);
            io_msg("a secret door");
            found = true;
            Game::level->set_real(x, y);
          }
          break;

        case FLOOR:
          if (!os_rand_range(2 + increased_difficulty)) {
            Game::level->set_ch(x, y, TRAP);

            if (is_hallucinating()) {
              io_msg(trap_names[os_rand_range(NTRAPS)].c_str());
            } else {
              io_msg(trap_names[Game::level->get_trap_type(x, y)].c_str());
              Game::level->set_discovered(x, y);
            }

            found = true;
            Game::level->set_discovered(x, y);
          }
          break;

        case SHADOW:
          if (!os_rand_range(3 + increased_difficulty)) {
            Game::level->set_ch(x, y, PASSAGE);
            found = true;
            Game::level->set_real(x, y);
          }
          break;
      }
    }
  }

  if (found) {
    look(false);
    player->set_not_running();
  }
}

void Player::restore_strength() {
  if (get_strength() < get_default_strength()) {
    Character::restore_strength();
    io_msg("you feel your strength returning");

  } else {
    io_msg("you feel warm all over");
  }
}

void Player::modify_strength(int amount) {

  int orig_amount = amount;
  int current_strength = get_strength();

  if (current_strength + amount < player_min_strength) {
    amount = player_min_strength - current_strength;
  } else if (current_strength + amount > player_max_strength) {
    amount = player_max_strength - current_strength;
  }

  Character::modify_strength(amount);
  current_strength = get_strength();

  if ((amount <= 0 != orig_amount <= 0) || current_strength < player_min_strength ||
      current_strength > player_max_strength) {
    error("Strength calculation error. Min: " + to_string(player_min_strength) +
          ". Max: " + to_string(player_max_strength) +
          ". Current: " + to_string(current_strength) +
          ". Amount: " + to_string(amount) +
          ". Orig amount: " + to_string(orig_amount));
  }
}


void Player::raise_level(int amount)
{
  if (amount <= 0) {
    error("Cannot raise 0 or less levels");
  }

  Character::raise_level(amount);
  io_msg("welcome to level %d", get_level());
}

void Player::check_for_level_up() {
  vector<int> levels = {
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

  int new_level;
  int old_level = get_level();

  for (new_level = 0; levels.at(static_cast<size_t>(new_level)) != 0; ++new_level) {
    if (levels.at(static_cast<size_t>(new_level)) > get_experience()) {
      break;
    }
  }

  ++new_level;
  if (new_level > old_level) {
    raise_level(new_level - old_level);
  }
}


bool Player::has_ring_with_ability(int ability) const {
  for (int i = 0; i < PACK_RING_SLOTS; ++i) {

    Item* ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring != nullptr && ring->o_which == ability)
      return true;
  }
  return false;
}

void Player::rust_armor() {
  Item* arm = pack_equipped_item(EQUIPMENT_ARMOR);
  if (arm == nullptr || arm->o_type != ARMOR || arm->o_which == Armor::Type::LEATHER ||
      arm->o_arm >= 9) {
    return;
  }

  if ((arm->o_flags & ISPROT) || has_ring_with_ability(Ring::Type::SUSTARM)) {
    if (!to_death) {
      io_msg("the rust vanishes instantly");
    }
  }
  else {
    arm->o_arm++;
    io_msg("your armor weakens");
  }
}

void Player::set_previous_room(struct room* room) {
  previous_room = room;
}

room* Player::get_previous_room() const {
  return previous_room;
}

