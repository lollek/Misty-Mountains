#include "disk.h"
#include "os.h"
#include "misc.h"
#include "game.h"
#include "item/scrolls.h"

#include "character.h"

using namespace std;

Character::Character(int str_, int dex_, int con_, int int_, int wis_, int cha_,
    int exp_, int lvl_, int ac_, int hd_, std::vector<damage> const& attacks_,
    Coordinate const& position_, vector<Feat> const& feats_, int speed_, Race race_) :

  strength{str_}, default_strength{str_},
  dexterity{dex_}, default_dexterity{dex_},
  constitution{con_}, default_constitution{con_},
  intelligence{int_}, default_intelligence{int_},
  wisdom{con_}, default_wisdom{wis_},
  charisma{cha_}, default_charisma{cha_},

  experience{exp_}, level{lvl_}, base_ac{ac_}, hit_dice{hd_},
  base_health{hit_dice + roll(level-1, hit_dice)},
  health{get_max_health()},
  attacks{attacks_}, position{position_}, speed{speed_}, race{race_},
  feats{feats_}, turns_not_moved{0},

  confusing_attack{0}, true_sight{0}, blind{0}, cancelled{0}, levitating{0},
  found{0}, greedy{0}, players_target{0}, held{0}, confused{0},
  invisible{0}, mean{0}, regenerating{0}, running{0},
  stuck{0}, attack_freeze{0}, attack_damage_armor{0},
  attack_steal_gold{0}, attack_steal_item{0}, attack_drain_strength{0},
  attack_drain_health{0}, attack_drain_experience{0}
{}

int  Character::get_speed() const { return speed; }
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
bool Character::is_greedy() const { return greedy; }
bool Character::is_players_target() const { return players_target; }
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

bool Character::has_feat(Feat feat) const {
  return find(feats.begin(), feats.end(), feat) != feats.end();
}

bool Character::attacks_on_sight() const {
  return has_feat(AttacksOnSight);
}

void Character::add_feat(Feat feat) {
  if (!has_feat(feat)) {
    feats.push_back(feat);
  }
}

void Character::set_attacks_on_sight() {
  add_feat(AttacksOnSight);
}

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

  if (can_raise_max_health) {
    int extra_max_hp = 0;
    if (health > get_max_health() + level + 1)
      ++extra_max_hp;
    if (health > get_max_health())
      ++extra_max_hp;

    base_health += extra_max_hp;
  }

  if (health > get_max_health())
    health = get_max_health();
}

bool Character::is_hurt() const {
  return health != get_max_health();
}

void Character::modify_max_health(int amount) {
  base_health += amount;
  health += amount;
}

void Character::raise_level(int amount) {
  experience = 0;
  level += amount;

  // Roll extra HP
  modify_max_health(roll(amount, hit_dice));
}

void Character::lower_level(int amount) {
  level = max(1, level - amount);

  modify_max_health(-roll(amount, hit_dice));
}

int Character::get_level() const {
  return level;
}

int Character::get_ac() const {
  return base_ac + (dexterity - 10) / 2;
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
  return base_health + (constitution - 10) / 2;
}

Coordinate const& Character::get_position() const {
  return position;
}

vector<damage> const& Character::get_attacks() const {
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
void Character::set_running() { running = true; }
void Character::set_not_running() { running = false; }
void Character::set_chasing() { running = true; }
void Character::set_not_chasing() { running = false; }
void Character::set_not_stuck() { stuck = false; }
void Character::set_invisible() { invisible = true; }
void Character::set_position(Coordinate const& position_) {
  position = position_;
}

void Character::set_base_stats(int str_, int dex_, int con_,
                              int wis_, int int_, int cha_) {
  strength = default_strength = str_;
  dexterity = default_dexterity = dex_;
  constitution = default_constitution = con_;
  wisdom = default_wisdom = wis_;
  intelligence = default_intelligence = int_;
  charisma = default_charisma = cha_;
}


void Character::save(ostream& data) const {
  Disk::save_tag(TAG_CHARACTER, data);

  Disk::save(TAG_STATS, strength, data);
  Disk::save(TAG_STATS, default_strength, data);
  Disk::save(TAG_STATS, dexterity, data);
  Disk::save(TAG_STATS, default_dexterity, data);
  Disk::save(TAG_STATS, constitution, data);
  Disk::save(TAG_STATS, default_constitution, data);
  Disk::save(TAG_STATS, wisdom, data);
  Disk::save(TAG_STATS, default_wisdom, data);
  Disk::save(TAG_STATS, intelligence, data);
  Disk::save(TAG_STATS, default_intelligence, data);
  Disk::save(TAG_STATS, charisma, data);
  Disk::save(TAG_STATS, default_charisma, data);

  Disk::save(TAG_MISC, experience, data);
  Disk::save(TAG_MISC, level, data);
  Disk::save(TAG_MISC, base_ac, data);
  Disk::save(TAG_MISC, hit_dice, data);
  Disk::save(TAG_MISC, base_health, data);
  Disk::save(TAG_MISC, health, data);
  Disk::save(TAG_MISC, attacks, data);
  Disk::save(TAG_MISC, position, data);
  Disk::save(TAG_MISC, speed, data);
  Disk::save(TAG_MISC, static_cast<int const>(race), data);
  Disk::save(TAG_MISC, feats, data);
  Disk::save(TAG_MISC, turns_not_moved, data);

  Disk::save(TAG_FLAG, confusing_attack, data);
  Disk::save(TAG_FLAG, true_sight, data);
  Disk::save(TAG_FLAG, blind, data);
  Disk::save(TAG_FLAG, cancelled, data);
  Disk::save(TAG_FLAG, levitating, data);
  Disk::save(TAG_FLAG, found, data);
  Disk::save(TAG_FLAG, greedy, data);
  Disk::save(TAG_FLAG, players_target, data);
  Disk::save(TAG_FLAG, held, data);
  Disk::save(TAG_FLAG, confused, data);
  Disk::save(TAG_FLAG, invisible, data);
  Disk::save(TAG_FLAG, mean, data);
  Disk::save(TAG_FLAG, regenerating, data);
  Disk::save(TAG_FLAG, running, data);
  Disk::save(TAG_FLAG, stuck, data);
  Disk::save(TAG_FLAG, attack_freeze, data);
  Disk::save(TAG_FLAG, attack_damage_armor, data);
  Disk::save(TAG_FLAG, attack_steal_gold, data);
  Disk::save(TAG_FLAG, attack_steal_item, data);
  Disk::save(TAG_FLAG, attack_drain_strength, data);
  Disk::save(TAG_FLAG, attack_drain_health, data);
  Disk::save(TAG_FLAG, attack_drain_experience, data);

  Disk::save_tag(TAG_CHARACTER, data);
}

bool Character::load(istream& data) {
  if (!Disk::load_tag(TAG_CHARACTER, data) ||

      !Disk::load(TAG_STATS, strength, data) ||
      !Disk::load(TAG_STATS, default_strength, data) ||
      !Disk::load(TAG_STATS, dexterity, data) ||
      !Disk::load(TAG_STATS, default_dexterity, data) ||
      !Disk::load(TAG_STATS, constitution, data) ||
      !Disk::load(TAG_STATS, default_constitution, data) ||
      !Disk::load(TAG_STATS, wisdom, data) ||
      !Disk::load(TAG_STATS, default_wisdom, data) ||
      !Disk::load(TAG_STATS, intelligence, data) ||
      !Disk::load(TAG_STATS, default_intelligence, data) ||
      !Disk::load(TAG_STATS, charisma, data) ||
      !Disk::load(TAG_STATS, default_charisma, data) ||

      !Disk::load(TAG_MISC, experience, data) ||
      !Disk::load(TAG_MISC, level, data) ||
      !Disk::load(TAG_MISC, base_ac, data) ||
      !Disk::load(TAG_MISC, hit_dice, data) ||
      !Disk::load(TAG_MISC, base_health, data) ||
      !Disk::load(TAG_MISC, health, data) ||
      !Disk::load(TAG_MISC, attacks, data) ||
      !Disk::load(TAG_MISC, position, data) ||
      !Disk::load(TAG_MISC, speed, data) ||
      !Disk::load(TAG_MISC, reinterpret_cast<int&>(race), data) ||
      !Disk::load(TAG_MISC, feats, data) ||
      !Disk::load(TAG_MISC, turns_not_moved, data) ||

      !Disk::load(TAG_FLAG, confusing_attack, data) ||
      !Disk::load(TAG_FLAG, true_sight, data) ||
      !Disk::load(TAG_FLAG, blind, data) ||
      !Disk::load(TAG_FLAG, cancelled, data) ||
      !Disk::load(TAG_FLAG, levitating, data) ||
      !Disk::load(TAG_FLAG, found, data) ||
      !Disk::load(TAG_FLAG, greedy, data) ||
      !Disk::load(TAG_FLAG, players_target, data) ||
      !Disk::load(TAG_FLAG, held, data) ||
      !Disk::load(TAG_FLAG, confused, data) ||
      !Disk::load(TAG_FLAG, invisible, data) ||
      !Disk::load(TAG_FLAG, mean, data) ||
      !Disk::load(TAG_FLAG, regenerating, data) ||
      !Disk::load(TAG_FLAG, running, data) ||
      !Disk::load(TAG_FLAG, stuck, data) ||
      !Disk::load(TAG_FLAG, attack_freeze, data) ||
      !Disk::load(TAG_FLAG, attack_damage_armor, data) ||
      !Disk::load(TAG_FLAG, attack_steal_gold, data) ||
      !Disk::load(TAG_FLAG, attack_steal_item, data) ||
      !Disk::load(TAG_FLAG, attack_drain_strength, data) ||
      !Disk::load(TAG_FLAG, attack_drain_health, data) ||
      !Disk::load(TAG_FLAG, attack_drain_experience, data) ||

      !Disk::load_tag(TAG_CHARACTER, data)) {
    return false;
  }
  return true;
}

void Character::increase_speed() {
  if (++speed == 0) {
    speed = 1;
  }
  turns_not_moved = 0;
}

void Character::decrease_speed() {
  if (--speed) {
    speed = -1;
  }
  turns_not_moved = 0;
}


// If speed > 0, its the number of turns to take.
// Else it's a turn per 1 / (-speed +2) rounds. E.g. 0 -> 1/2, -1 -> 1/3
int Character::get_moves_this_round() {
  if (speed > 0) {
    return speed;
  }

  if (turns_not_moved > -speed) {
    turns_not_moved = 0;
    return 1;
  }

  turns_not_moved++;
  return 0;
}

int Character::get_dexterity() const {
  return dexterity;
}

int Character::get_default_dexterity() const {
  return default_dexterity;
}

int Character::get_constitution() const {
  return constitution;
}

int Character::get_default_constitution() const {
  return default_constitution;
}

int Character::get_wisdom() const {
  return wisdom;
}

int Character::get_default_wisdom() const {
  return default_wisdom;
}

int Character::get_intelligence() const {
  return intelligence;
}

int Character::get_default_intelligence() const {
  return default_intelligence;
}

int Character::get_charisma() const {
  return charisma;
}

int Character::get_default_charisma() const {
  return default_charisma;
}

Character::Race Character::get_race() const {
  return race;
}

string Character::race_to_string(Race race) {
  switch (race) {
    case Human:             return "human";
    case Dwarf:             return "dwarf";
    case Elf:               return "elf";
    case Animal:            return "animal";
    case Reptilian:         return "reptilian";
    case Goblinoid:         return "goblinoid";
    case Elemental:         return "elemental";
    case Fey:               return "fey";
    case Orc:               return "orc";
    case Undead:            return "undead";
    case Humanoid:          return "humanoid";
    case MonstrousHumanoid: return "monstrous humanoid";
    case Aberration:        return "aberration";
    case MagicBeast:        return "magic beast";
    case Dragon:            return "dragon";
    case Outsider:          return "outsider";
    case Giant:             return "giant";
  }
}
