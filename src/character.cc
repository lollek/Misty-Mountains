#include "os.h"
#include "misc.h"
#include "game.h"
#include "scrolls.h"

#include "character.h"

using namespace std;

Character::Character(int strength_, int experience_, int level_, int armor_,
    int health_, std::vector<damage> const& attacks_,
    Coordinate const& position_, struct room* room_, int flags_, char type_) :
  strength(strength_), experience(experience_), level(level_), armor(armor_),
  health(health_), attacks(attacks_), max_health(health), position(position_),
  room(room_), flags(flags_), type(type_)
{}

bool Character::is_blind() const {
  return flags & ISBLIND;
}

bool Character::is_cancelled() const {
  return flags & ISCANC;
}

bool Character::is_confused() const {
  return flags & ISHUH;
}

bool Character::has_confusing_attack() const {
  return flags & CANHUH;
}

bool Character::is_found() const {
  return flags & ISFOUND;
}

bool Character::is_hallucinating() const {
  return flags & ISHALU;
}

bool Character::is_invisible() const {
  return flags & ISINVIS;
}

bool Character::is_levitating() const {
  return flags & ISLEVIT;
}

bool Character::has_true_sight() const {
  return flags & CANSEE;
}

bool Character::is_held() const {
  return flags & ISHELD;
}

bool Character::is_stuck() const {
  return flags & ISSTUCK;
}

bool Character::is_chasing() const {
  return flags & ISRUN;
}

bool Character::is_mean() const {
  return flags & ISMEAN;
}

bool Character::is_greedy() const {
  return flags & ISGREED;
}

bool Character::is_players_target() const {
  return flags & ISTARGET;
}

bool Character::is_slowed() const {
  return flags & ISSLOW;
}

bool Character::is_hasted() const {
  return flags & ISHASTE;
}

bool Character::is_flying() const {
  return flags & ISFLY;
}

void Character::set_blind() {
  flags |= ISBLIND;
}

void Character::set_cancelled() {
  flags |= ISCANC;
}

void Character::set_confused() {
  flags |= ISHUH;
}

void Character::set_confusing_attack() {
  flags |= CANHUH;
}

void Character::set_found() {
  flags |= ISFOUND;
}

void Character::set_hallucinating() {
  flags |= ISHALU;
}

void Character::set_levitating() {
  flags |= ISLEVIT;
}

void Character::set_true_sight() {
  flags |= CANSEE;
}

void Character::set_stuck() {
  flags |= ISSTUCK;
}

void Character::set_not_blind() {
  flags &= ~ISBLIND;
}

void Character::set_not_cancelled() {
  flags &= ~ISCANC;
}

void Character::set_not_confused() {
  flags &= ~ISHUH;
}

void Character::remove_confusing_attack() {
  flags &= ~CANHUH;
}

void Character::set_not_found() {
  flags &= ~ISFOUND;
}

void Character::set_not_hallucinating() {
  flags &= ~ISHALU;
}

void Character::set_not_invisible() {
  flags &= ~ISINVIS;
}

void Character::set_not_levitating() {
  flags &= ~ISLEVIT;
}

void Character::remove_true_sight() {
  flags &= ~CANSEE;
}

void Character::set_held() {
  flags &= ~ISRUN;
  flags |= ISHELD;
}

void Character::set_not_held() {
  flags &= ~ISHELD;
}

void Character::take_damage(int damage) {
  health -= damage;
}

void Character::seroom(struct room* new_room) {
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

  // Now check to see if that's a legal move.
  // If not, don't move.(I.e., bump into the wall or whatever)
  if (!diag_ok(&position, &ret)) {
    ret.x = position.x;
    ret.y = position.y;
    return ret;
  }

  char ch = Game::level->get_type(x, y);
  if (!step_ok(ch)) {
    ret.x = position.x;
    ret.y = position.y;
    return ret;
  }

  if (ch == SCROLL) {
    Item* item = Game::level->get_item(x, y);
    if (item != nullptr && item->o_which == S_SCARE) {
      ret.x = position.x;
      ret.y = position.y;
      return ret;
    }
  }

  return ret;
}

void Character::restore_strength() {
  strength = default_strength;
}

void Character::modify_strength(int amount) {
  // Negative is temporary and Positive is permanent (if not below default).
  if (amount <= 0) {
    strength -= amount;
  }

  if (strength == default_strength) {
    strength = default_strength = strength + amount;
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
