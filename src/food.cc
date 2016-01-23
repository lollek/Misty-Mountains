#include "command.h"
#include "io.h"
#include "pack.h"
#include "player.h"
#include "rings.h"
#include "os.h"
#include "death.h"

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

void
food_eat(void)
{
  if (food_left < 0)
    food_left = 0;

  food_left += satiated - 200 + os_rand_range(400);

  if (food_left > full)
    food_left = full;

  hunger_state = 0;
}

static int
ring_drain_amount(void)
{
  int total_eat = 0;
  int uses[] = {
    1, /* R_PROTECT */  1, /* R_ADDSTR   */  1, /* R_SUSTSTR  */
    1, /* R_SEARCH  */  1, /* R_SEEINVIS */  0, /* R_NOP      */
    0, /* R_AGGR    */  1, /* R_ADDHIT   */  1, /* R_ADDDAM   */
    2, /* R_REGEN   */ -1, /* R_DIGEST   */  0, /* R_TELEPORT */
    1, /* R_STEALTH */  1, /* R_SUSTARM  */
  };

  for (int i = 0; i < PACK_RING_SLOTS; ++i)
  {
    Item *ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring != nullptr)
      total_eat += uses[ring->o_which];
  }

  return total_eat;
}

void
food_digest(void)
{
  int old_food_left = food_left;
  food_left -= 1 + ring_drain_amount() - pack_contains_amulet();

  if (food_left < hunger_alert && old_food_left >= hunger_alert)
  {
    hunger_state = HUNGRY;
    io_msg("you are starting to get hungry");
    command_stop(true);
  }

  if (food_left < starvation_alert && old_food_left >= starvation_alert)
  {
    hunger_state = WEAK;
    io_msg("you feel weak from lack of food");
    command_stop(true);
  }

  if (food_left < starvation_start)
  {
    if (player_turns_without_action || os_rand_range(5) != 0)
      return;

    player_turns_without_action += os_rand_range(8) + 4;
    hunger_state = STARVING;
    io_msg("you faint from lack of food");
    command_stop(true);
  }

  if (food_left < starvation_death)
    death(DEATH_HUNGER);
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
