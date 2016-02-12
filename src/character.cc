#include "disk.h"
#include "os.h"
#include "misc.h"
#include "game.h"
#include "scrolls.h"

#include "character.h"

using namespace std;

Character::Character(int strength_, int experience_, int level_, int armor_,
    int health_, std::vector<damage> const& attacks_,
    Coordinate const& position_, unsigned long long flags, char type_) :
  strength(strength_), default_strength(strength),experience(experience_),
  level(level_), armor(armor_), health(health_), attacks(attacks_),
  max_health(health), position(position_), type(type_),
  confusing_attack(0), true_sight(0), blind(0), cancelled(0), levitating(0),
  found(0), greedy(0), players_target(0), held(0), confused(0),
  invisible(0), mean(0), regenerating(0), running(0),
  flying(0), stuck(0), attack_freeze(0), attack_damage_armor(0),
  attack_steal_gold(0), attack_steal_item(0), attack_drain_strength(0),
  attack_drain_health(0), attack_drain_experience(0)
{
  if (flags & 010000000000000000000) { greedy = true; }
  if (flags & 020000000000000000000) { mean = true; }
  if (flags & 040000000000000000000) { flying = true; }
  if (flags & 001000000000000000000) { regenerating = true; }
  if (flags & 002000000000000000000) { invisible = true; }
  if (flags & 004000000000000000000) { attack_freeze = true; }
  if (flags & 000100000000000000000) { attack_damage_armor = true; }
  if (flags & 000200000000000000000) { attack_steal_gold = true; }
  if (flags & 000400000000000000000) { attack_steal_item = true; }
  if (flags & 000010000000000000000) { attack_drain_strength = true; }
  if (flags & 000020000000000000000) { attack_drain_health = true; }
  if (flags & 000040000000000000000) { attack_drain_experience = true; }
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
bool Character::is_flying() const { return flying; }
bool Character::attack_freezes() const { return attack_freeze; }
bool Character::attack_damages_armor() const { return attack_damage_armor; }
bool Character::attack_steals_gold() const { return attack_steal_gold; }
bool Character::attack_steals_item() const { return attack_steal_item; }
bool Character::attack_drains_strength() const { return attack_drain_strength; }
bool Character::attack_drains_health() const { return attack_drain_health; }
bool Character::attack_drains_experience() const { return attack_drain_experience; }

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


void Character::save(ofstream& data) {
  Disk::save_tag(TAG_CHARACTER, data);
  Disk::save(TAG_CHARACTER, strength, data);
  Disk::save(TAG_CHARACTER, default_strength, data);
  Disk::save(TAG_CHARACTER, experience, data);
  Disk::save(TAG_CHARACTER, level, data);
  Disk::save(TAG_CHARACTER, armor, data);
  Disk::save(TAG_CHARACTER, health, data);
  Disk::save(TAG_CHARACTER, attacks, data);
  Disk::save(TAG_CHARACTER, max_health, data);
  Disk::save(TAG_CHARACTER, position, data);
  Disk::save(TAG_CHARACTER, type, data);

  Disk::save(TAG_CHARACTER, confusing_attack, data);
  Disk::save(TAG_CHARACTER, true_sight, data);
  Disk::save(TAG_CHARACTER, blind, data);
  Disk::save(TAG_CHARACTER, cancelled, data);
  Disk::save(TAG_CHARACTER, levitating, data);
  Disk::save(TAG_CHARACTER, found, data);
  Disk::save(TAG_CHARACTER, greedy, data);
  Disk::save(TAG_CHARACTER, players_target, data);
  Disk::save(TAG_CHARACTER, held, data);
  Disk::save(TAG_CHARACTER, confused, data);
  Disk::save(TAG_CHARACTER, invisible, data);
  Disk::save(TAG_CHARACTER, mean, data);
  Disk::save(TAG_CHARACTER, regenerating, data);
  Disk::save(TAG_CHARACTER, running, data);
  Disk::save(TAG_CHARACTER, flying, data);
  Disk::save(TAG_CHARACTER, stuck, data);
  Disk::save(TAG_CHARACTER, attack_freeze, data);
  Disk::save(TAG_CHARACTER, attack_damage_armor, data);
  Disk::save(TAG_CHARACTER, attack_steal_gold, data);
  Disk::save(TAG_CHARACTER, attack_steal_item, data);
  Disk::save(TAG_CHARACTER, attack_drain_strength, data);
  Disk::save(TAG_CHARACTER, attack_drain_health, data);
  Disk::save(TAG_CHARACTER, attack_drain_experience, data);
}

bool Character::load(ifstream& data) {
  if (!Disk::load_tag(TAG_CHARACTER, data) ||
      !Disk::load(TAG_CHARACTER, strength, data) ||
      !Disk::load(TAG_CHARACTER, default_strength, data) ||
      !Disk::load(TAG_CHARACTER, experience, data) ||
      !Disk::load(TAG_CHARACTER, level, data) ||
      !Disk::load(TAG_CHARACTER, armor, data) ||
      !Disk::load(TAG_CHARACTER, health, data) ||
      !Disk::load(TAG_CHARACTER, attacks, data) ||
      !Disk::load(TAG_CHARACTER, max_health, data) ||
      !Disk::load(TAG_CHARACTER, position, data) ||
      !Disk::load(TAG_CHARACTER, type, data) ||

      !Disk::load(TAG_CHARACTER, confusing_attack, data) ||
      !Disk::load(TAG_CHARACTER, true_sight, data) ||
      !Disk::load(TAG_CHARACTER, blind, data) ||
      !Disk::load(TAG_CHARACTER, cancelled, data) ||
      !Disk::load(TAG_CHARACTER, levitating, data) ||
      !Disk::load(TAG_CHARACTER, found, data) ||
      !Disk::load(TAG_CHARACTER, greedy, data) ||
      !Disk::load(TAG_CHARACTER, players_target, data) ||
      !Disk::load(TAG_CHARACTER, held, data) ||
      !Disk::load(TAG_CHARACTER, confused, data) ||
      !Disk::load(TAG_CHARACTER, invisible, data) ||
      !Disk::load(TAG_CHARACTER, mean, data) ||
      !Disk::load(TAG_CHARACTER, regenerating, data) ||
      !Disk::load(TAG_CHARACTER, running, data) ||
      !Disk::load(TAG_CHARACTER, flying, data) ||
      !Disk::load(TAG_CHARACTER, stuck, data) ||
      !Disk::load(TAG_CHARACTER, attack_freeze, data) ||
      !Disk::load(TAG_CHARACTER, attack_damage_armor, data) ||
      !Disk::load(TAG_CHARACTER, attack_steal_gold, data) ||
      !Disk::load(TAG_CHARACTER, attack_steal_item, data) ||
      !Disk::load(TAG_CHARACTER, attack_drain_strength, data) ||
      !Disk::load(TAG_CHARACTER, attack_drain_health, data) ||
      !Disk::load(TAG_CHARACTER, attack_drain_experience, data)) {
    return false;
  }
  return true;
}
