#include <vector>

#include "command.h"
#include "death.h"
#include "game.h"
#include "item/food.h"
#include "os.h"

#include "player.h"

using namespace std;

enum HungerState {
  Normal,
  Hungry,
  Weak,
  Starving,
};

static int const full{5000};
static int const satiated{3500};
static int const hunger_alert{1000};
static int const starvation_alert{300};
static int const starvation_start{0};
static int const starvation_death{-1000};
static HungerState hunger_state{HungerState::Normal};

void Player::eat(Food* food) {
  if (food->get_type() == Food::Fruit) {
    Game::io->message("my, that was a yummy fruit");

  } else if (os_rand_range(100) > 70) {
    gain_experience(1);
    Game::io->message("this food tastes awful");
    check_for_level_up();

  } else {
    Game::io->message("that tasted good");
  }

  if (nutrition_left < 0) {
    nutrition_left = 0;
  }

  nutrition_left += food->get_nutrition_value();

  if (nutrition_left > full) {
    nutrition_left = full;
  }

  hunger_state = HungerState::Normal;
}

void Player::digest_food() {
  int const old_nutrition_left{nutrition_left};
  nutrition_left -=
      1 + player->equipment_food_drain_amount() - pack_contains_amulet();

  if (nutrition_left < hunger_alert && old_nutrition_left >= hunger_alert) {
    hunger_state = HungerState::Hungry;
    Game::io->message("you are starting to get hungry");
    command_stop(true);
  }

  if (nutrition_left < starvation_alert &&
      old_nutrition_left >= starvation_alert) {
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
    case Normal: return "Satiated";
    case Hungry: return "Hungry";
    case Weak: return "Weak";
    case Starving: return "Faint";
  }
}

int Player::get_nutrition_left() const { return nutrition_left; }

int Player::get_starting_nutrition() { return satiated; }
