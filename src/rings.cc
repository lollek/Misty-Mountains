#include <ctype.h>
#include <string.h>

#include <vector>
#include <string>

using namespace std;

#include "daemons.h"
#include "io.h"
#include "item.h"
#include "misc.h"
#include "options.h"
#include "os.h"
#include "pack.h"
#include "player.h"
#include "rogue.h"
#include "things.h"
#include "weapons.h"
#include "monster.h"

#include "rings.h"

static vector<string> r_stones;

static struct stone {
    string st_name;
    size_t st_value;
} stones[] = {
  { "agate",		 25},
  { "alexandrite",	 40},
  { "amethyst",	 50},
  { "carnelian",	 40},
  { "diamond",	300},
  { "emerald",	300},
  { "germanium",	225},
  { "granite",	  5},
  { "garnet",		 50},
  { "jade",		150},
  { "kryptonite",	300},
  { "lapis lazuli",	 50},
  { "moonstone",	 50},
  { "obsidian",	 15},
  { "onyx",		 60},
  { "opal",		200},
  { "pearl",		220},
  { "peridot",	 63},
  { "ruby",		350},
  { "sapphire",	285},
  { "stibotantalite",	200},
  { "tiger eye",	 50},
  { "topaz",		 60},
  { "turquoise",	 70},
  { "taaffeite",	300},
  { "zircon",	 	 80},
};
static int NSTONES = (sizeof(stones) / sizeof(*stones));

vector<obj_info> ring_info = {
  { "protection",		 9, 400,   "", false },
  { "add strength",		 9, 400,   "", false },
  { "sustain strength",		 5, 280,   "", false },
  { "searching",		10, 420,   "", false },
  { "see invisible",		10, 310,   "", false },
  { "adornment",		 1,  10,   "", false },
  { "aggravate monster",	10,  10,   "", false },
  { "dexterity",		 8, 440,   "", false },
  { "increase damage",		 8, 400,   "", false },
  { "regeneration",		 4, 460,   "", false },
  { "slow digestion",		 9, 240,   "", false },
  { "teleportation",		 5,  30,   "", false },
  { "stealth",			 7, 470,   "", false },
  { "maintain armor",		 5, 380,   "", false },
};

void
ring_init(void)
{
  for (size_t i = 0; i < NRINGS; i++)
    for (;;)
    {
      int stone = os_rand_range(NSTONES);

      if (find(begin(r_stones), end(r_stones), stones[stone].st_name) == end(r_stones))
        continue;

      r_stones.push_back(stones[stone].st_name);
      ring_info.at(i).oi_worth += stones[stone].st_value;
      break;
    }
}


bool
ring_put_on(void)
{
  item* obj = pack_get_item("put on", RING);

  /* Make certain that it is somethings that we want to wear */
  if (obj == nullptr)
    return false;

  if (obj->o_type != RING)
  {
    io_msg("not a ring");
    return ring_put_on();
  }

  /* Try to put it on */
  if (!pack_equip_item(obj))
  {
    io_msg("you already have a ring on each hand");
    return false;
  }
  pack_remove(obj, false, true);

  /* Calculate the effect it has on the poor guy. */
  switch (obj->o_which)
  {
    case R_ADDSTR: player_modify_strength(obj->o_arm); break;
    case R_SEEINVIS: invis_on(); break;
    case R_AGGR: monster_aggravate_all(); break;
    }

  char buf[MAXSTR];
  ring_description(obj, buf);
  buf[0] = static_cast<char>(tolower(buf[0]));
  io_msg("now wearing %s", buf);
  return true;
}

bool
ring_take_off(void)
{
  enum equipment_pos ring;

  /* Try right, then left */
  if (pack_equipped_item(EQUIPMENT_RRING) != nullptr)
    ring = EQUIPMENT_RRING;
  else
    ring = EQUIPMENT_LRING;

  item* obj = pack_equipped_item(ring);

  if (!pack_unequip(ring, false))
    return false;

  switch (obj->o_which)
  {
    case R_ADDSTR:
      player_modify_strength(-obj->o_arm);
      break;

    case R_SEEINVIS:
      player_remove_true_sight(0);
      daemon_extinguish_fuse(player_remove_true_sight);
      break;
  }
  return true;
}

int
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
    item *ring = pack_equipped_item(pack_ring_slots[i]);
    if (ring != nullptr)
      total_eat += uses[ring->o_which];
  }

  return total_eat;
}

bool
ring_is_known(enum ring_t ring)
{
  return ring_info[ring].oi_know;
}

void
ring_description(item const* item, char* buf)
{
  obj_info* op = &ring_info.at(static_cast<size_t>(item_subtype(item)));
  buf += sprintf(buf, "%s ring", r_stones.at(static_cast<size_t>(item_subtype(item))).c_str());

  if (op->oi_know)
  {
    buf += sprintf(buf, " of %s", op->oi_name.c_str());
    switch (item_subtype(item))
    {
      case R_PROTECT: case R_ADDSTR: case R_ADDDAM: case R_ADDHIT:
        if (item_armor(item) > 0)
          buf += sprintf(buf, " [+%d]", item_armor(item));
        else
          buf += sprintf(buf, " [%d]", item_armor(item));
        break;
      default: break;
    }
  }
  else if (!op->oi_guess.empty())
    sprintf(buf, " called %s", op->oi_guess.c_str());
}

item*
ring_create(int which, bool random_stats)
{
  if (which == -1)
    which = static_cast<int>(pick_one(ring_info, NRINGS));

  item* ring = new item();
  ring->o_type = RING;
  ring->o_which = which;

  switch (ring->o_which)
  {
    case R_ADDSTR: case R_PROTECT: case R_ADDHIT: case R_ADDDAM:
      if (random_stats)
      {
        ring->o_arm = os_rand_range(3);
        if (ring->o_arm == 0)
        {
          ring->o_arm = -1;
          ring->o_flags |= ISCURSED;
        }
      }
      else
        ring->o_arm = 1;
      break;

    case R_AGGR: case R_TELEPORT:
      ring->o_flags |= ISCURSED;
      break;
  }

  return ring;
}

