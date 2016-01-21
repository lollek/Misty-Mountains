#include <string>
#include <vector>
#include <sstream>

using namespace std;

#include "error_handling.h"
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

Armor::Armor(bool random_stats) :
  Armor(armor_type_random(), random_stats)
{}

Armor::Armor(Armor::Type type, bool random_stats) {
  o_type = ARMOR;
  o_which = type;
  o_arm = armor_ac(type);

  if (random_stats) {
    int rand = os_rand_range(100);
    if (rand < 20) {
      o_flags |= ISCURSED;
      o_arm += os_rand_range(3) + 1;
    }
    else if (rand < 28) {
      o_arm -= os_rand_range(3) + 1;
    }
  }
}

static vector<armor_info_t const> armors {
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

string const& armor_name(Armor::Type i)  { return armors.at(i).name; }
int armor_ac(Armor::Type i)              { return armors.at(i).ac; }
int armor_value(Armor::Type i)           { return armors.at(i).value; }
int armor_probability(Armor::Type i)     { return armors.at(i).prob; }

bool
armor_command_wear() {
  Item* obj = pack_get_item("wear", ARMOR);

  if (obj == nullptr) {
    return false;
  }

  if (obj->o_type != ARMOR) {
    io_msg("you can't wear that");
    return armor_command_wear();
  }

  if (pack_equipped_item(EQUIPMENT_ARMOR) != nullptr) {
    if (!pack_unequip(EQUIPMENT_ARMOR, false)) {
      return true;
    }
  }

  player->waste_time(1);
  pack_remove(obj, false, true);
  pack_equip_item(obj);

  io_msg("now wearing %s", inv_name(obj, true).c_str());
  return true;
}

Armor::Type
armor_type_random() {
  int value = os_rand_range(100);
  for (Armor::Type i = static_cast<Armor::Type>(0);
       i < Armor::Type::NARMORS;
       i = static_cast<Armor::Type>(static_cast<int>(i) + 1)) {
    if (value < armors[i].prob) {
      return i;
    } else {
      value -= armors[i].prob;
    }
  }

  error("Error! Sum of probabilities is not 100%");
}

void
armor_rust() {
  Item* arm = pack_equipped_item(EQUIPMENT_ARMOR);
  if (arm == nullptr || arm->o_type != ARMOR || arm->o_which == Armor::Type::LEATHER ||
      arm->o_arm >= 9) {
    return;
  }

  if ((arm->o_flags & ISPROT) || player->has_ring_with_ability(R_SUSTARM)) {
    if (!to_death) {
      io_msg("the rust vanishes instantly");
    }
  }
  else {
    arm->o_arm++;
    io_msg("your armor weakens");
  }
}

string
armor_description(Item const* item) {
  stringstream buffer;

  string const& obj_name = armor_name(static_cast<Armor::Type>(item_subtype(item)));
  int bonus_ac = armor_ac(static_cast<Armor::Type>(item_subtype(item))) -item_armor(item);
  int base_ac = 10 - item_armor(item) - bonus_ac;

  buffer << "A" << vowelstr(obj_name) << " " <<obj_name << " [" << base_ac;

  if (item_is_known(item)) {
    buffer << ",";
    if (bonus_ac > 0) {
      buffer << "+";
    }
    buffer << bonus_ac;
  }
  buffer << "]";

  if (!item->get_nickname().empty()) {
    buffer << " called " << item->get_nickname();
  }

  return buffer.str();
}

