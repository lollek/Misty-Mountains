#include "os.h"
#include "misc.h"
#include "game.h"
#include "scrolls.h"

#include "character.h"

using namespace std;

Character::Character(int strength_, int experience_, int level_, int armor_,
    int health_, std::vector<damage> const& attacks_,
    Coordinate const& position_, struct room* room_, int flags, char type_) :
  strength(strength_), default_strength(strength),experience(experience_),
  level(level_), armor(armor_), health(health_), attacks(attacks_),
  max_health(health), position(position_), room(room_), type(type_),
  confusing_attack(0), true_sight(0), blind(0), cancelled(0), levitating(0),
  found(0), greedy(0), hasted(0), players_target(0), held(0), confused(0),
  invisible(0), mean(0), regenerating(0), running(0),
  flying(0), slowed(0), stuck(0)
{
  if (flags & 0000001) { confusing_attack = true; }
  if (flags & 0000002) { true_sight = true; }
  if (flags & 0000004) { blind = true; }
  if (flags & 0000010) { blind = true; levitating = true; }
  if (flags & 0000020) { found = true; }
  if (flags & 0000040) { greedy = true; }
  if (flags & 0000100) { hasted = true; }
  if (flags & 0000200) { players_target = true; }
  if (flags & 0000400) { held = true; }
  if (flags & 0001000) { confused = true; }
  if (flags & 0002000) { invisible = true; }
  if (flags & 0004000) { mean = true; }
  if (flags & 0010000) { regenerating = true; }
  if (flags & 0020000) { running = true; }
  if (flags & 0040000) { flying = true; }
  if (flags & 0100000) { slowed = true; }
  if (flags & 0200000) { stuck = true; }
}

bool Character::is_blind() const { return blind; }
bool Character::is_cancelled() const { return cancelled; }
bool Character::is_confused() const { return confused; }
bool Character::has_confusing_attack() const { return confusing_attack; }
bool Character::is_found() const { return found; }
bool Character::is_invisible() const { return invisible; }
bool Character::is_levitating() const { return levitating; }
bool Character::has_true_sight() const { return true_sight; }
bool Character::is_held() const { return held; }
bool Character::is_stuck() const { return stuck; }
bool Character::is_chasing() const { return running; }
bool Character::is_running() const { return running; }
bool Character::is_mean() const { return mean; }
bool Character::is_greedy() const { return greedy; }
bool Character::is_players_target() const { return players_target; }
bool Character::is_slowed() const { return slowed; }
bool Character::is_hasted() const { return hasted; }
bool Character::is_flying() const { return flying; }

void Character::set_blind() { blind = true; }
void Character::set_cancelled() { cancelled = true; }
void Character::set_confused() { confused = true; }
void Character::set_confusing_attack() { confusing_attack = true; }
void Character::set_found() { found = true; }
void Character::set_levitating() { levitating = true; }
void Character::set_true_sight() { true_sight = true; }
void Character::set_stuck() { stuck = true; }
void Character::set_not_blind() { blind = false; }
void Character::set_not_cancelled() { cancelled = false; }
void Character::set_not_confused() { confused = false; }
void Character::remove_confusing_attack() { confusing_attack = false; }
void Character::set_not_found() { found = false; }
void Character::set_not_invisible() { invisible = false; }
void Character::set_not_levitating() { levitating = false; }
void Character::remove_true_sight() { true_sight = false; }
void Character::set_not_held() { held = false; }

void Character::set_held() {
  held = true;
  running = false;
}

void Character::take_damage(int damage) {
  health -= damage;
}

void Character::set_room(struct room* new_room) {
  room = new_room;
}

Coordinate Character::possible_random_move() {
  Coordinate ret;

  // Generate a random
  int x = ret.x = position.x + os_rand_range(3) - 1;
  int y = ret.y = position.y + os_rand_range(3) - 1;

  if (y == position.y && x == position.x) {
    return ret;
  }

  if (!Game::level->can_step(x, y)) {
    ret.x = position.x;
    ret.y = position.y;
    return ret;
  }

  Item* item = Game::level->get_item(x, y);
  if (item != nullptr && item->o_type == IO::Scroll && item->o_which == Scroll::SCARE) {
    ret.x = position.x;
    ret.y = position.y;
    return ret;
  }

  return ret;
}

void Character::restore_strength() {
  strength = default_strength;
}

void Character::modify_strength(int amount) {
  // Negative is temporary and Positive is permanent (if not below default).
  strength += amount;

  if (strength > default_strength) {
    default_strength = strength;
  }
}

int Character::get_default_strength() const {
  return default_strength;
}

void Character::restore_health(int amount, bool can_raise_max_health) {
  health += amount;

  if (can_raise_max_health)
  {
    int extra_max_hp = 0;
    if (health > max_health + level + 1)
      ++extra_max_hp;
    if (health > max_health)
      ++extra_max_hp;

    max_health += extra_max_hp;
  }

  if (health > max_health)
    health = max_health;
}

bool Character::is_hurt() const {
  return health != max_health;
}

void Character::modify_max_health(int amount) {
  max_health += amount;
  health += amount;
}

void Character::raise_level(int amount) {
  // Reset expometer
  experience = 0;

  // Raise levels
  level += amount;

  // Roll extra HP
  modify_max_health(roll(amount, 10));
}

void Character::lower_level(int amount) {
  experience = 0;
  level = max(1, level - amount);
}

int Character::get_level() const {
  return level;
}

int Character::get_armor() const {
  return armor;
}

int Character::get_type() const {
  return type;
}

room* Character::get_room() const {
  return room;
}

int Character::get_experience() const {
  return experience;
}

int Character::get_strength() const {
  return strength;
}

int Character::get_health() const {
  return health;
}

int Character::get_max_health() const {
  return max_health;
}

Coordinate const& Character::get_position() const {
  return position;
}

std::vector<damage> const& Character::get_attacks() const {
  return attacks;
}

void Character::set_mean() { mean = true; }
void Character::set_players_target() { players_target = true; }
void Character::set_not_players_target() { players_target = false; }
void Character::gain_experience(int experience_) {
  experience += experience_;
}

void Character::set_not_mean() { mean = false; }
void Character::set_greedy() { greedy = true; }
void Character::set_not_greedy() { greedy = false; }
void Character::set_slowed() { slowed = true; }
void Character::set_not_slowed() { slowed = false; }
void Character::set_hasted() { hasted = true; }
void Character::set_not_hasted() { hasted = false; }
void Character::set_flying() { flying = true; }
void Character::set_not_flying() { flying = false; }
void Character::set_running() { running = true; }
void Character::set_not_running() { running = false; }
void Character::set_chasing() { running = true; }
void Character::set_not_chasing() { running = false; }
void Character::set_not_stuck() { stuck = false; }
void Character::set_invisible() { invisible = true; }
void Character::set_position(Coordinate const& position_) {
  position = position_;
}

