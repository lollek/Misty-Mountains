#include <string>
#include <cmath>

#include "disk.h"
#include "command_private.h"
#include "food.h"
#include "error_handling.h"
#include "game.h"
#include "coordinate.h"
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

/* Duration of effects */
#define HUHDURATION     spread(20)  /* Confusion */
#define MFINDDURATION   spread(20)  /* Monster find */
#define HASTEDURATION   os_rand_range(4)+4    /* Haste */
#define SEEDURATION     spread(850) /* See invisible / blind */
#define LEVITDUR        spread(30)  /* Levitation */
#define SLEEPTIME       spread(7)   /* Sleep */
#define STUCKTIME       spread(3)   /* Stuck */



Player::Player(bool give_equipment) :
  Character(0, 0, 0, 0, 0, 0, 0, 1, 10, 12, {{1,4}}, Coordinate(), 0, 1),
  previous_room(nullptr), senses_monsters(false), senses_magic(false),
  pack(), equipment(equipment_size(), nullptr), gold(0),
  nutrition_left(get_starting_nutrition()) {

  if (!give_equipment) {
    return;
  }

  /* Give him some food */
  pack_add(new Food(), true, false);

  /* And his suit of armor */
  class Armor* armor_ = new class Armor(Armor::Softleatherarmor, false);
  armor_->set_identified();
  equipment.at(Armor) = armor_;

  /* Give him his weaponry.  First a mace. */
  class Weapon* dagger = new class Weapon(Weapon::Dagger, false);
  dagger->set_identified();
  equipment.at(Weapon) = dagger;
}

int Player::get_ac() const {
  int ac = Character::get_ac();

  class Armor* arm = equipped_armor();
  if (arm != nullptr) {
    ac += arm->get_armor();
  }

  // If weapon help protection, add it
  class Weapon* weapon = equipped_weapon();
  if (weapon != nullptr) {
    ac += weapon->get_armor();
  }

  // If rings help, add their stats as well
  for (Equipment position : all_rings()) {
    Item const* ring = equipment.at(static_cast<size_t>(position));
    if (ring != nullptr && ring->o_which == Ring::Protection) {
      ac += ring->get_armor();
    }
  }

  return ac;
}

bool Player::can_see(Monster const& monster) const {
  if (monster.is_invisible() && !has_true_sight()) {
    return false;
  }
  return can_see(monster.get_position());
}

bool Player::can_see(Coordinate const& coord) const {
  if (is_blind()) {
    return false;
  }

  Coordinate const& player_pos = get_position();
  int real_distance = Game::level->is_dark(coord) ? darkvision : lightvision;

  // dist is a**2 + b**2, and we dont wanna sqrt if we dont have to
  int see_distance = real_distance * real_distance;
  int dist_result = dist(coord.y, coord.x, player_pos.y, player_pos.x);
  if (dist_result >= see_distance) {
    return false;
  }

  // Trace the rays, and return false if something blocks
  Coordinate walker = player->get_position();
  double distance = sqrt(dist_result);
  double dx = (coord.x - walker.x) / distance;
  double dy = (coord.y - walker.y) / distance;

  for (double i = 1; i < distance; ++i) {
    Coordinate temp(static_cast<int>(round(walker.x + dx * i)),
                    static_cast<int>(round(walker.y + dy * i)));

    if (temp == coord) {
      return true;
    }

    Tile::Type tile = Game::level->get_tile(temp);
    switch (tile) {
      case Tile::Wall: case Tile::ClosedDoor:
        return false;

      case Tile::Floor: case Tile::StairsDown: case Tile::StairsUp: 
      case Tile::Trap: case Tile::OpenDoor: case Tile::Shop:
        break;
    }
  }
  return true;
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
    Daemons::daemon_run_before();
    Daemons::daemon_run_after();
  }
}


int Player::get_strength_with_bonuses() const {
  int bonuses = 0;
  for (Equipment position : all_rings()) {
    Item const* ring = equipment.at(static_cast<size_t>(position));
    if (ring != nullptr && ring->o_which == Ring::Strength) {
      bonuses += ring->get_armor();
    }
  }
  return get_strength() + bonuses;
}

bool Player::saving_throw(int which) const {
  if (which == VS_MAGIC) {
    for (Equipment position : all_rings()) {
      Item const* ring = equipment.at(static_cast<size_t>(position));
      if (ring != nullptr && ring->o_which == Ring::Protection) {
        which += ring->get_armor();
      }
    }
  }

  int need = 14 + which - get_level() / 2;
  return (roll(1, 20) >= need);
}

bool Player::has_true_sight() const {
  return Character::has_true_sight() ||
    player->has_ring_with_ability(Ring::SeeInvisible);
}

void Player::set_true_sight() {
  if (Character::has_true_sight()) {
    Daemons::daemon_lengthen_fuse(Daemons::remove_true_sight, SEEDURATION);

  } else {
    Character::set_true_sight();
    Daemons::daemon_start_fuse(Daemons::remove_true_sight, SEEDURATION, AFTER);
  }
}

void Player::set_confused() {
  if (is_confused()) {
    Daemons::daemon_lengthen_fuse(Daemons::set_not_confused, HUHDURATION);

  } else {
    Character::set_confused();
    Daemons::daemon_start_fuse(Daemons::set_not_confused, HUHDURATION, AFTER);
  }
  Game::io->message("wait, what's going on here. Huh? What? Who?");
}

void Player::set_not_confused() {
  Character::set_not_confused();
  Game::io->message("you feel less confused now");
}

bool Player::can_sense_magic() const {
  return senses_magic;
}

void Player::set_sense_magic() {
  Daemons::daemon_start_fuse(Daemons::remove_sense_magic, MFINDDURATION, AFTER);

  Game::io->message("you can smell magic in the air");
  senses_magic = true;
}

void Player::remove_sense_magic() {
  senses_magic = false;
  Game::io->message("you can no longer smell magic");
}

bool Player::can_sense_monsters() const {
  return senses_monsters;
}

void Player::set_sense_monsters() {
  Daemons::daemon_start_fuse(Daemons::remove_sense_monsters, MFINDDURATION, AFTER);

  Game::io->message("you can sense monsters nearby");
  senses_monsters = true;
}

void Player::remove_sense_monsters() {
  senses_monsters = false;
  Game::io->message("you can no longer sense any monsters");
}

void Player::increase_speed() {
  Character::increase_speed();
  Daemons::daemon_start_fuse(Daemons::decrease_speed, HASTEDURATION, AFTER);
  Game::io->message("you feel yourself moving much faster");
}

void Player::decrease_speed() {
  Character::decrease_speed();
  Game::io->message("you feel yourself slowing down");
}

void Player::set_blind() {

  if (is_blind()) {
    Daemons::daemon_lengthen_fuse(Daemons::set_not_blind, SEEDURATION);

  } else {
    Character::set_blind();
    Daemons::daemon_start_fuse(Daemons::set_not_blind, SEEDURATION, AFTER);
    Game::io->message("a cloak of darkness falls around you");
  }
}

void Player::set_not_blind() {

  if (!is_blind())
    return;

  Daemons::daemon_extinguish_fuse(Daemons::set_not_blind);
  Character::set_not_blind();
  Game::io->message("the veil of darkness lifts");
}

void Player::set_levitating() {
  if (is_levitating()) {
    Daemons::daemon_lengthen_fuse(Daemons::set_not_levitating, LEVITDUR);

  } else {
    Character::set_levitating();
    Daemons::daemon_start_fuse(Daemons::set_not_levitating, LEVITDUR, AFTER);
    Game::io->message("you start to float in the air");
  }
}

void Player::set_not_levitating() {
  if (!is_levitating()) {
    return;
  }

  Character::set_not_levitating();
  Game::io->message("you float gently to the ground");
}

void Player::set_confusing_attack()
{
  Character::set_confusing_attack();
  Game::io->message("your hands begin to glow red");
}

void Player::fall_asleep() {
  player_turns_without_action += SLEEPTIME;
  set_not_running();
  Game::io->message("you fall asleep");
}

void Player::become_stuck() {
  player_turns_without_moving += STUCKTIME;
  set_not_running();
}

void Player::become_poisoned() {
  if (player->has_ring_with_ability(Ring::SustainStrenght)) {
    Game::io->message("you feel momentarily nauseous");

  } else {
    modify_strength(-(os_rand_range(3) + 1));
    Game::io->message("you feel very sick now");
  }
}

bool Player::is_stealthy() const {
  return player->has_ring_with_ability(Ring::Stealth)
    || is_levitating();
}

void Player::teleport(Coordinate const* target)
{
  Coordinate new_pos;
  Coordinate const old_pos = get_position();

  // Set target location (nullptr means we generate a random position)
  if (target == nullptr) {
    do {
      Game::level->get_random_room_coord(nullptr, &new_pos, 0, true);
    } while (new_pos == old_pos);

  } else {
    new_pos.y = target->y;
    new_pos.x = target->x;
  }

  set_position(new_pos);

  if (is_held()) {
    set_not_held();
  }

  player_turns_without_moving = 0;
  command_stop(true);
  Game::io->message("suddenly you're somewhere else");
}

void Player::search() {
  int increased_difficulty = is_blind() ? 2 : 0;

  bool found = false;
  Coordinate const& player_pos = get_position();
  for (int y = player_pos.y - 1; y <= player_pos.y + 1; y++) {
    for (int x = player_pos.x - 1; x <= player_pos.x + 1; x++) {

      // If it's real, dont bother
      if (Game::level->is_real(x, y)) {
        continue;
      }

      // If fake, give player a chance to discover it
      switch (Game::level->get_tile(x, y)) {

        case Tile::Wall:
          if (!os_rand_range(5 + increased_difficulty)) {
            Game::level->set_tile(x, y, Tile::ClosedDoor);
            Game::io->message("a secret door");
            found = true;
            Game::level->set_real(x, y);
          }
          break;

        case Tile::Floor:
          if (!os_rand_range(2 + increased_difficulty)) {
            Game::level->set_tile(x, y, Tile::Trap);
            Game::io->message(Trap::name(static_cast<Trap::Type>(Game::level->get_trap_type(x, y))));
            Game::level->set_discovered(x, y);

            found = true;
            Game::level->set_discovered(x, y);
          }
          break;

        case Tile::OpenDoor: case Tile::ClosedDoor: case Tile::Trap:
        case Tile::StairsDown: case Tile::StairsUp: case Tile::Shop: break;
      }
    }
  }

  if (found) {
    player->set_not_running();
  }
}

void Player::restore_strength() {
  if (get_strength() < get_default_strength()) {
    Character::restore_strength();
    Game::io->message("you feel your strength returning");

  } else {
    Game::io->message("you feel warm all over");
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
  Game::io->message("welcome to level " + to_string(get_level()));
}

void Player::check_for_level_up() {
  vector<int> levels = {
//   1    2    3    4     5    6     7     8     9     10    11
    10L, 25L, 45L, 70L, 100L, 140L, 200L, 280L, 380L, 500L, 650L,
//   12    13     14     15     16     17     18     19     20
    850L, 1100L, 1400L, 1800L, 2300L, 2900L, 3600L, 4400L, 5400L,
//   21    22     23     24     25     26     27     28     29
    6800L,8400L,10200L,12500L,17500L,25000L,35000L,50000L,75000L,
//   30        31       32       33      34       35
   100000L, 150000L, 200000L, 300000L, 400000L, 500000L,
//   36        37       38        39
   750000L, 1500000L, 2500000L, 5000000L
  };

  if (get_level() >= 40) {
    return;
  }

  int experience_to_next_level = levels.at(static_cast<size_t>(get_level()));
  int current_experience = get_experience();

  if (experience_to_next_level < current_experience) {
    raise_level(1);
  }
}


bool Player::has_ring_with_ability(int ability) const {
  for (Equipment position : all_rings()) {
    Item const* ring = equipment.at(static_cast<size_t>(position));
    if (ring != nullptr && ring->o_which == ability)
      return true;
  }
  return false;
}

void Player::rust_armor() {
  class Armor* arm = dynamic_cast<class Armor*>(equipment.at(static_cast<size_t>(Armor)));
  if (arm == nullptr) {
    return;
  }

  if (arm->is_rustproof()) {
    if (!to_death) {
      Game::io->message("the rust vanishes instantly");
    }
  }
  else {
    arm->modify_armor(-1);
    Game::io->message("your armor weakens");
  }
}

void Player::set_previous_room(struct room* room) {
  previous_room = room;
}

room* Player::get_previous_room() const {
  return previous_room;
}

vector<Equipment> Player::all_rings() {
  return {Ring1, Ring2};
}

int Player::equipment_food_drain_amount() {
  int total_eat = 0;
  vector<int> uses {
    1, /* R_PROTECT */  1, /* R_ADDSTR   */  1, /* R_SUSTSTR  */
    1, /* R_SEARCH  */  1, /* R_SEEINVIS */  0, /* R_NOP      */
    0, /* R_AGGR    */  1, /* R_ADDHIT   */  1, /* R_ADDDAM   */
    2, /* R_REGEN   */ -1, /* R_DIGEST   */  0, /* R_TELEPORT */
    1, /* R_STEALTH */  1, /* R_SUSTARM  */
  };

  for (Equipment position : all_rings()) {
    Item *ring = equipment.at(position);
    if (ring != nullptr) {
      total_eat += uses.at(static_cast<size_t>(ring->o_which));
    }
  }

  return total_eat;
}

void Player::pack_uncurse() {
  for (Item* item : pack) {
    item->set_not_cursed();
  }

  for (Item* equip : equipment) {
    if (equip != nullptr) {
      equip->set_not_cursed();
    }
  }
}

void Player::save_player(ostream& data) {
  Disk::save_tag(TAG_PLAYER, data);
  Character* c_player = dynamic_cast<Character*>(player);
  c_player->save(data);
  Disk::save(TAG_INVENTORY,       player->pack,            data);
  Disk::save(TAG_EQUIPMENT,       player->equipment,       data);
  Disk::save(TAG_SENSES_MONSTERS, player->senses_monsters, data);
  Disk::save(TAG_SENSES_MAGIC,    player->senses_magic,    data);
  Disk::save(TAG_GOLD,            player->gold,            data);
  Disk::save(TAG_NUTRITION,       player->nutrition_left,  data);
}

void Player::load_player(istream& data) {
  Disk::load_tag(TAG_PLAYER, data);
  player = new Player(false);
  Character* c_player = static_cast<Character*>(player);
  if (!c_player->load(data) ||
      !Disk::load(TAG_INVENTORY,       player->pack,            data) ||
      !Disk::load(TAG_EQUIPMENT,       player->equipment,       data) ||
      !Disk::load(TAG_SENSES_MONSTERS, player->senses_monsters, data) ||
      !Disk::load(TAG_SENSES_MAGIC,    player->senses_magic,    data) ||
      !Disk::load(TAG_GOLD,            player->gold,            data) ||
      !Disk::load(TAG_NUTRITION,       player->nutrition_left,  data)) {
    error("No player character found");
  }
}
