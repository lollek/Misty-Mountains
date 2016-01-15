#include <stdlib.h>
#include <string.h>

#include "io.h"
#include "item.h"
#include "misc.h"
#include "options.h"
#include "os.h"
#include "pack.h"
#include "player.h"
#include "rings.h"
#include "rogue.h"
#include "things.h"

#include "armor.h"

static struct armor_info_t armors[NARMORS] = {
 /* name                   ac  prob value known */
 { "leather armor",         8, 20,   20,  false },
 { "ring mail",             7, 15,   25,  false },
 { "studded leather armor", 7, 15,   20,  false },
 { "scale mail",            6, 13,   30,  false },
 { "chain mail",            5, 12,   75,  false },
 { "splint mail",           4, 10,   80,  false },
 { "banded mail",           4, 10,   90,  false },
 { "plate mail",            3,  5,  150,  false },
};

char const* armor_name(enum armor_t i)  { return armors[i].name; }
int armor_ac(enum armor_t i)            { return armors[i].ac; }
int armor_value(enum armor_t i)         { return armors[i].value; }
int armor_probability(enum armor_t i)   { return armors[i].prob; }

int
armor_for_monster(monster const* mon) {
  return is_player(mon) ? player_get_armor() : mon->t_stats.s_arm;
}

bool
armor_command_wear() {
  item* obj = pack_get_item("wear", ARMOR);

  if (obj == NULL)
    return false;

  if (obj->o_type != ARMOR) {
    io_msg("you can't wear that");
    return armor_command_wear();
  }

  if (pack_equipped_item(EQUIPMENT_ARMOR) != NULL)
    if (!pack_unequip(EQUIPMENT_ARMOR, false))
      return true;

  waste_time(1);
  pack_remove(obj, false, true);
  pack_equip_item(obj);

  char buf[MAXSTR];
  io_msg("now wearing %s", inv_name(buf, obj, true));
  return true;
}

enum armor_t
armor_type_random() {
  int value = os_rand_range(100);
  for (enum armor_t i = static_cast<armor_t>(0);
       i < NARMORS;
       i = static_cast<armor_t>(static_cast<int>(i) + 1)) {
    if (value < armors[i].prob)
      return i;
    else
      value -= armors[i].prob;
  }

  /* Error! Sum of probs was not 100 */
  io_debug("Error! Sum of probabilities is not 100%", 0);
  io_readchar(false);
  return LEATHER;
}

void
armor_rust() {
  item* arm = pack_equipped_item(EQUIPMENT_ARMOR);
  if (arm == NULL || arm->o_type != ARMOR || arm->o_which == LEATHER ||
      arm->o_arm >= 9)
    return;

  if ((arm->o_flags & ISPROT) || player_has_ring_with_ability(R_SUSTARM)) {
    if (!to_death)
      io_msg("the rust vanishes instantly");
  }
  else {
    arm->o_arm++;
    io_msg("your armor weakens");
  }
}

void
armor_description(item const* item, char* buf) {
  char *ptr = buf;
  char const* obj_name = armor_name(static_cast<armor_t>(item_subtype(item)));
  int bonus_ac = armor_ac(static_cast<armor_t>(item_subtype(item))) -item_armor(item);
  int base_ac = 10 - item_armor(item) - bonus_ac;

  ptr += sprintf(ptr, "A%s %s [%d]", vowelstr(obj_name).c_str(), obj_name, base_ac);

  if (item_is_known(item)) {
    ptr -= 1;
    ptr += sprintf(ptr, bonus_ac < 0 ? ",%d]" : ",+%d]", bonus_ac);
  }

  if (!item_nickname(item).empty())
    ptr += sprintf(ptr, " called %s", item_nickname(item).c_str());
}

item*
armor_create(int which, int random_stats) {
  if (which == -1)
    which = armor_type_random();

  item* armor = new item();
  armor->o_type = ARMOR;
  armor->o_which = which;
  armor->o_arm = armor_ac(static_cast<armor_t>(armor->o_which));

  if (random_stats) {
    int rand = os_rand_range(100);
    if (rand < 20) {
      armor->o_flags |= ISCURSED;
      armor->o_arm += os_rand_range(3) + 1;
    }
    else if (rand < 28)
      armor->o_arm -= os_rand_range(3) + 1;
  }

  return armor;
}
