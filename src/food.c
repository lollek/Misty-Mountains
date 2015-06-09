#include "command.h"
#include "io.h"
#include "misc.h"
#include "pack.h"
#include "player.h"
#include "rings.h"
#include "rip.h"
#include "state.h"

#include "food.h"

enum hunger_state
{
  NORMAL,
  HUNGRY,
  WEAK,
  STARVING,
};

static char const* hunger_state_name[] = {
  "",
  "Hungry",
  "Weak",
  "Faint"
};

#define STARTING_FOOD 1300

static int const full = 2000;
static int const satiated = 1300;
static int const hunger_alert = 300;
static int const starvation_alert = 150;
static int const starvation_start = 0;
static int const starvation_death = -850;

static int food_left = STARTING_FOOD;
static int hunger_state = 0;

bool
food_save_state(void)
{
  return state_save_int32(food_left)
    || state_save_int32(hunger_state);
}

bool
food_load_state(void)
{
  return state_load_int32(&food_left)
    || state_load_int32(&hunger_state);
}

void
food_eat(void)
{
  if (food_left < 0)
    food_left = 0;

  food_left += satiated - 200 + rnd(400);

  if (food_left > full)
    food_left = full;

  hunger_state = 0;
}

void
food_digest(void)
{
  int old_food_left = food_left;
  food_left -= 1 + ring_drain_amount() - pack_contains_amulet();

  if (food_left < hunger_alert && old_food_left >= hunger_alert)
  {
    hunger_state = HUNGRY;
    msg("you are starting to get hungry");
    command_stop(true);
  }

  if (food_left < starvation_alert && old_food_left >= starvation_alert)
  {
    hunger_state = WEAK;
    msg("you feel weak from lack of food");
    command_stop(true);
  }

  if (food_left < starvation_start)
  {
    if (player_turns_without_action || rnd(5) != 0)
      return;

    player_turns_without_action += rnd(8) + 4;
    hunger_state = STARVING;
    msg("you faint from lack of food");
    command_stop(true);
  }

  if (food_left < starvation_death)
    death('s');
}

int
food_nutrition_left(void)
{
  return food_left;
}

char const*
food_hunger_state(void)
{
  return hunger_state_name[hunger_state];
}
