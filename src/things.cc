#include <vector>
#include <string>
#include <sstream>

using namespace std;

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
  stringstream buffer;

  switch (item->o_type)
  {
    case POTION: buffer << potion_description(item); break;
    case RING:   buffer << ring_description(item); break;
    case STICK:  buffer << wand_description(item); break;
    case SCROLL: buffer << scroll_description(item); break;
    case WEAPON: buffer << weapon_description(item); break;
    case AMMO: buffer << weapon_description(item); break;
    case ARMOR: buffer << armor_description(item); break;
    case FOOD: {
      string obj_type = item->o_which == 1 ? "fruit" : "food ration";
      if (item->o_count == 1) {
        buffer << "A " << obj_type;
      } else {
        buffer << item->o_count << " " << obj_type;
      }
    } break;
    case AMULET: buffer << "The Amulet of Yendor"; break;
    case GOLD: buffer << item->o_goldval << " Gold pieces"; break;

    default:
      io_msg("You feel a disturbance in the force");
      buffer << "Something bizarre " << unctrl(static_cast<chtype>(item->o_type));
      break;
  }

  string return_value = buffer.str();
  return_value.at(0) = static_cast<char>(drop
      ? tolower(return_value.at(0))
      : toupper(return_value.at(0)));
  return return_value;
}

Item*
new_food(int which)
{
  /* Reset levels-without-food counter */
  Game::levels_without_food = 0;

  Item* cur = new Item();
  cur->o_count = 1;
  cur->o_type = FOOD;
  switch (which)
  {
    case 0: case 1: cur->o_which = which; break;
    default: cur->o_which = os_rand_range(10) ? 0 : 1; break;
  }
  return cur;
}

Item*
new_amulet(void)
{
  Item* obj       = new Item();
  obj->o_damage  = {1, 2};
  obj->o_hurldmg = {1, 2};
  obj->o_type    = AMULET;

  return obj;
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
    case 2: cur = new_food(-1); break;
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

