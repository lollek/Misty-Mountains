#include <vector>
#include <string>
#include <sstream>

#include "food.h"
#include "amulet.h"
#include "gold.h"
#include "error_handling.h"
#include "game.h"
#include "coordinate.h"
#include "potions.h"
#include "scrolls.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "rings.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "weapons.h"
#include "wand.h"
#include "os.h"
#include "options.h"
#include "rogue.h"

#include "things.h"

using namespace std;


/* Only oi_prob is used */
vector<obj_info> things = {
/*   oi_name oi_prob oi_worth oi_guess oi_know */
    { "potion",	26,	0,	"",	false },
    { "scroll",	36,	0,	"",	false },
    { "food",	16,	0,	"",	false },
    { "weapon",	 7,	0,	"",	false },
    { "armor",	 7,	0,	"",	false },
    { "ring",	 4,	0,	"",	false },
    { "stick",	 4,	0,	"",	false },
};

string
inv_name(Item const* item, bool drop)
{
  string return_value;

  switch (item->o_type)
  {
    case POTION: return_value = potion_description(item); break;
    case RING:   return_value = ring_description(item); break;
    case STICK:  return_value = wand_description(item); break;
    case SCROLL: return_value = scroll_description(item); break;
    case WEAPON: return_value = weapon_description(item); break;
    case AMMO:   return_value = weapon_description(item); break;
    case ARMOR:  return_value = armor_description(item); break;
    case FOOD:   return_value = food_description(item); break;
    case AMULET: return_value = amulet_description(item); break;
    case GOLD:   return_value = gold_description(item); break;

    default:
      error("Unknown type: " + string(1, item->o_type));
  }

  return_value.at(0) = static_cast<char>(drop
      ? tolower(return_value.at(0))
      : toupper(return_value.at(0)));
  return return_value;
}

Item*
new_thing(void)
{
  /* Decide what kind of object it will be
   * If we haven't had food for a while, let it be food. */
  int r;
  if (Game::levels_without_food > 3)
    r = 2;
  else
    r = static_cast<int>(pick_one(things, things.size()));

  Item* cur = nullptr;
  switch (r)
  {
    case 0: cur = new Potion(); break;
    case 1: cur = new Scroll(); break;
    case 2: cur = new Food(); break;
    case 3: cur = weapon_create(-1, true); break;
    case 4: cur = new Armor(true); break;
    case 5: cur = new Ring(true); break;
    case 6: cur = new Wand(); break;
    default:
      io_msg("Picked a bad kind of object (this should not happen)");
      io_wait_for_key(KEY_SPACE);
      break;
  }

  if (cur == nullptr) {
    error("curr is null");
  }
  if (cur->o_damage.sides < 0 || cur->o_damage.dices < 0) {
    error("cur->damage is negative");
  }
  if (cur->o_hurldmg.sides < 0 || cur->o_hurldmg.dices < 0) {
    error("cur->hurldmg is negative");
  }
  return cur;
}

size_t
pick_one(vector<obj_info>& start, size_t nitems)
{
  for (size_t rand = static_cast<size_t>(os_rand_range(100)), i = 0; i < nitems; ++i)
    if (rand < start.at(i).oi_prob)
      return i;
    else
      rand -= start.at(i).oi_prob;

  /* The functions should have returned by now */
  error("Probability was not in range");
}

