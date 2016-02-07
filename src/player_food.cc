#include <vector>

#include "death.h"
#include "command.h"
#include "game.h"
#include "os.h"
#include "pack.h"

#include "player.h"

using namespace std;

enum HungerState {
  Normal,
  Hungry,
  Weak,
  Starving,
};

static int const full = 5000;
static int const satiated = 3500;
static int const hunger_alert = 1000;
static int const starvation_alert = 300;
static int const starvation_start = 0;
static int const starvation_death = -1000;
static HungerState hunger_state = HungerState::Normal;

void Player::eat() {
  if (nutrition_left < 0) {
    nutrition_left = 0;
  }

  nutrition_left += satiated - 200 + os_rand_range(400);

  if (nutrition_left > full) {
    nutrition_left = full;
  }

  hunger_state = HungerState::Normal;
}

static int ring_drain_amount() {
  int total_eat = 0;
  vector<int> uses {
    1, /* R_PROTECT */  1, /* R_ADDSTR   */  1, /* R_SUSTSTR  */
    1, /* R_SEARCH  */  1, /* R_SEEINVIS */  0, /* R_NOP      */
    0, /* R_AGGR    */  1, /* R_ADDHIT   */  1, /* R_ADDDAM   */
    2, /* R_REGEN   */ -1, /* R_DIGEST   */  0, /* R_TELEPORT */
    1, /* R_STEALTH */  1, /* R_SUSTARM  */
  };

  for (int i = 0; i < PACK_RING_SLOTS; ++i) {
    Item *ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring != nullptr) {
      total_eat += uses.at(static_cast<size_t>(ring->o_which));
    }
  }

  return total_eat;
}

void Player::digest_food() {
  int old_nutrition_left = nutrition_left;
  nutrition_left -= 1 + ring_drain_amount() - pack_contains_amulet();

  if (nutrition_left < hunger_alert && old_nutrition_left >= hunger_alert) {
    hunger_state = HungerState::Hungry;
    Game::io->message("you are starting to get hungry");
    command_stop(true);
  }

  if (nutrition_left < starvation_alert && old_nutrition_left >= starvation_alert) {
    hunger_state = HungerState::Weak;
    Game::io->message("you feel weak from lack of food");
    command_stop(true);
  }

  if (nutrition_left < starvation_start) {
    if (player_turns_without_action || os_rand_range(5) != 0) {
      return;
    }

    player_turns_without_action += os_rand_range(8) + 4;
    hunger_state = HungerState::Starving;
    Game::io->message("you faint from lack of food");
    command_stop(true);
  }

  if (nutrition_left < starvation_death) {
    death(DEATH_HUNGER);
  }
}

string Player::get_hunger_state() const {
  switch (hunger_state) {
    case Normal:   return "Satiated";
    case Hungry:   return "Hungry";
    case Weak:     return "Weak";
    case Starving: return "Faint";
  }
}

int Player::get_nutrition_left() const {
  return nutrition_left;
}

int Player::get_starting_nutrition() {
  return satiated;
}
