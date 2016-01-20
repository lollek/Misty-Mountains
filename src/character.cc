#include "os.h"
#include "misc.h"
#include "game.h"
#include "scrolls.h"

#include "character.h"

using namespace std;

Character::Character(int strength, int experience, int level, int armor, int health,
    std::vector<damage> const& attacks, Coordinate const& position,
    room* room, int flags, char type) :
  s_str(strength), s_exp(experience), s_lvl(level), s_arm(armor), s_hpt(health),
  s_dmg(attacks), s_maxhp(health), t_pos(position), t_room(room),
  t_flags(flags), t_type(type)
{}

bool Character::is_blind() const {
  return t_flags & ISBLIND;
}

bool Character::is_cancelled() const {
  return t_flags & ISCANC;
}

bool Character::is_confused() const {
  return t_flags & ISHUH;
}

bool Character::has_confusing_attack() const {
  return t_flags & CANHUH;
}

bool Character::is_found() const {
  return t_flags & ISFOUND;
}

bool Character::is_hallucinating() const {
  return t_flags & ISHALU;
}

bool Character::is_invisible() const {
  return t_flags & ISINVIS;
}

bool Character::is_levitating() const {
  return t_flags & ISLEVIT;
}

bool Character::has_true_sight() const {
  return t_flags & CANSEE;
}

bool Character::is_held() const {
  return t_flags & ISHELD;
}

bool Character::is_stuck() const {
  return t_flags & ISSTUCK;
}

bool Character::is_chasing() const {
  return t_flags & ISRUN;
}

bool Character::is_mean() const {
  return t_flags & ISMEAN;
}

bool Character::is_greedy() const {
  return t_flags & ISGREED;
}

bool Character::is_players_target() const {
  return t_flags & ISTARGET;
}

bool Character::is_slowed() const {
  return t_flags & ISSLOW;
}

bool Character::is_hasted() const {
  return t_flags & ISHASTE;
}

bool Character::is_flying() const {
  return t_flags & ISFLY;
}

void Character::set_blind() {
  t_flags |= ISBLIND;
}

void Character::set_cancelled() {
  t_flags |= ISCANC;
}

void Character::set_confused() {
  t_flags |= ISHUH;
}

void Character::set_confusing_attack() {
  t_flags |= CANHUH;
}

void Character::set_found() {
  t_flags |= ISFOUND;
}

void Character::set_hallucinating() {
  t_flags |= ISHALU;
}

void Character::set_levitating() {
  t_flags |= ISLEVIT;
}

void Character::set_true_sight() {
  t_flags |= CANSEE;
}

void Character::set_stuck() {
  t_flags |= ISSTUCK;
}

void Character::set_not_blind() {
  t_flags &= ~ISBLIND;
}

void Character::set_not_cancelled() {
  t_flags &= ~ISCANC;
}

void Character::set_not_confused() {
  t_flags &= ~ISHUH;
}

void Character::remove_confusing_attack() {
  t_flags &= ~CANHUH;
}

void Character::set_not_found() {
  t_flags &= ~ISFOUND;
}

void Character::set_not_hallucinating() {
  t_flags &= ~ISHALU;
}

void Character::set_not_invisible() {
  t_flags &= ~ISINVIS;
}

void Character::set_not_levitating() {
  t_flags &= ~ISLEVIT;
}

void Character::remove_true_sight() {
  t_flags &= ~CANSEE;
}

void Character::set_held() {
  t_flags &= ~ISRUN;
  t_flags |= ISHELD;
}

void Character::set_not_held() {
  t_flags &= ~ISHELD;
}

void Character::take_damage(int damage) {
  s_hpt -= damage;
}

void Character::set_room(room* new_room) {
  t_room = new_room;
}

Coordinate Character::possible_random_move() {
  Coordinate ret;

  // Generate a random
  int x = ret.x = t_pos.x + os_rand_range(3) - 1;
  int y = ret.y = t_pos.y + os_rand_range(3) - 1;

  if (y == t_pos.y && x == t_pos.x) {
    return ret;
  }

  // Now check to see if that's a legal move.
  // If not, don't move.(I.e., bump into the wall or whatever)
  if (!diag_ok(&t_pos, &ret)) {
    ret.x = t_pos.x;
    ret.y = t_pos.y;
    return ret;
  }

  char ch = Game::level->get_type(x, y);
  if (!step_ok(ch)) {
    ret.x = t_pos.x;
    ret.y = t_pos.y;
    return ret;
  }

  if (ch == SCROLL) {
    Item* item = Game::level->get_item(x, y);
    if (item != nullptr && item->o_which == S_SCARE) {
      ret.x = t_pos.x;
      ret.y = t_pos.y;
      return ret;
    }
  }

  return ret;
}

void Character::restore_strength() {
  s_str = s_defstr;
}

void Character::modify_strength(int amount) {
  // Negative is temporary and Positive is permanent (if not below default).
  if (amount <= 0) {
    s_str -= amount;
  }

  if (s_str == s_defstr) {
    s_str = s_defstr = s_str + amount;
  }
}

int Character::get_default_strength() const {
  return s_defstr;
}

void Character::restore_health(int amount, bool can_raise_max_health) {
  s_hpt += amount;

  if (can_raise_max_health)
  {
    int extra_max_hp = 0;
    if (s_hpt > s_maxhp + s_lvl + 1)
      ++extra_max_hp;
    if (s_hpt > s_maxhp)
      ++extra_max_hp;

    s_maxhp += extra_max_hp;
  }

  if (s_hpt > s_maxhp)
    s_hpt = s_maxhp;
}

bool Character::is_hurt() const {
  return s_hpt != s_maxhp;
}

void Character::modify_max_health(int amount) {
  s_maxhp += amount;
  s_hpt += amount;
}

void Character::raise_level(int amount) {
  // Reset expometer
  s_exp = 0;

  // Raise levels
  s_lvl += amount;

  // Roll extra HP
  modify_max_health(roll(amount, 10));
}

void Character::lower_level(int amount) {
  s_exp = 0;
  s_lvl = max(1, s_lvl - amount);
}

