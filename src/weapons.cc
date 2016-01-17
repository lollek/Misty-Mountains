#include <vector>
#include <string>
#include <sstream>

using namespace std;

#include "error_handling.h"
#include "game.h"
#include "coordinate.h"
#include "fight.h"
#include "io.h"
#include "item.h"
#include "level.h"
#include "misc.h"
#include "monster.h"
#include "options.h"
#include "os.h"
#include "pack.h"
#include "player.h"
#include "rogue.h"
#include "things.h"

#include "weapons.h"

static Item* last_wielded_weapon = nullptr;

#define NO_WEAPON '\0'

static struct init_weaps {
    damage const iw_dam;		/* Damage when wielded */
    damage const iw_hrl;		/* Damage when thrown */
    char         iw_launch;	/* Launching weapon */
    int          iw_flags;	/* Miscellaneous flags */
} init_dam[MAXWEAPONS] = {
    { {2,4}, {1,3}, NO_WEAPON,  0,             },	/* Mace */
    { {3,4}, {1,2}, NO_WEAPON,  0,             },	/* Long sword */
    { {1,1}, {2,3}, NO_WEAPON,  0,             },	/* Bow */
    { {0,0}, {2,3}, BOW,        ISMANY|ISMISL, },	/* Arrow */
    { {1,6}, {1,4}, NO_WEAPON,  ISMISL,        },	/* Dagger */
    { {4,4}, {1,2}, NO_WEAPON,  0,             },	/* 2h sword */
    { {0,0}, {1,3}, NO_WEAPON,  ISMANY|ISMISL, },	/* Dart */
    { {0,0}, {2,4}, NO_WEAPON,  ISMANY|ISMISL, },	/* Shuriken */
    { {2,3}, {1,6}, NO_WEAPON,  ISMISL,        },	/* Spear */
};

vector<obj_info> weapon_info = {
    { "mace",				11,   8, "", false },
    { "long sword",			11,  15, "", false },
    { "short bow",			12,  15, "", false },
    { "arrow",				12,   1, "", false },
    { "dagger",				 8,   3, "", false },
    { "two handed sword",		10,  75, "", false },
    { "dart",				12,   2, "", false },
    { "shuriken",			12,   5, "", false },
    { "spear",				12,   5, "", false },
    /* DO NOT REMOVE: fake entry for dragon's breath */
    { "",				0,    0, "", false },	
};

void
weapon_missile_fall(Item* obj, bool pr) {
  Coordinate fpos;

  if (fallpos(&obj->get_pos(), &fpos)) {
    PLACE* pp = Game::level->get_place(fpos);
    pp->p_ch = static_cast<char>(obj->o_type);
    obj->set_pos(fpos);

    if (cansee(fpos.y, fpos.x)) {
      if (pp->p_monst != nullptr) {
        pp->p_monst->t_oldch = static_cast<char>(obj->o_type);
      } else {
        mvaddcch(fpos.y, fpos.x, static_cast<chtype>(obj->o_type));
      }
    }

    level_items.push_back(obj);
    return;
  }

  if (pr) {
    io_msg("the %s vanishes as it hits the ground",
        weapon_info.at(static_cast<size_t>(obj->o_which)).oi_name.c_str());
  }
  delete obj;
}

Item*
weapon_create(int which, bool random_stats) {
  if (which == -1) {
    which = static_cast<int>(pick_one(weapon_info, MAXWEAPONS));
  }

  Item* weap = new Item();
  weap->o_type  = WEAPON;
  weap->o_which = which;

  struct init_weaps* iwp = &init_dam[which];
  weap->o_launch     = iwp->iw_launch;
  weap->o_flags      = iwp->iw_flags;
  weap->o_damage     = iwp->iw_dam;
  weap->o_hurldmg    = iwp->iw_hrl;

  if (weap->o_flags & ISMANY) {
    weap->o_type = AMMO;
  }

  if (which == SPEAR) {
    weap->o_arm = 2;
  }

  if (which == DAGGER) {
    weap->o_count = os_rand_range(4) + 2;
  } else if (weap->o_flags & ISMANY) {
    weap->o_count = os_rand_range(8) + 8;
  } else {
    weap->o_count = 1;
  }

  if (random_stats) {
    int rand = os_rand_range(100);
    if (rand < 10) {
      weap->o_flags |= ISCURSED;
      weap->o_hplus -= os_rand_range(3) + 1;
    }
    else if (rand < 15) {
      weap->o_hplus += os_rand_range(3) + 1;
    }
  }

  return weap;
}

bool
weapon_wield(Item* weapon) {
  Item* currently_wielding = pack_equipped_item(EQUIPMENT_RHAND);
  if (currently_wielding != nullptr) {
    if (!pack_unequip(EQUIPMENT_RHAND, true)) {
      return true;
    }
  }

  pack_remove(weapon, false, true);
  pack_equip_item(weapon);

  io_msg("wielding %s", inv_name(weapon, true).c_str());
  last_wielded_weapon = currently_wielding;
  return true;
}

void
weapon_set_last_used(Item* weapon) {
  last_wielded_weapon = weapon;
}

bool
weapon_wield_last_used(void) {

  if (last_wielded_weapon == nullptr || !pack_contains(last_wielded_weapon)) {
    last_wielded_weapon = nullptr;
    io_msg("you have no weapon to switch to");
    return false;
  }

  return weapon_wield(last_wielded_weapon);
}

string
weapon_description(Item const* item) {
  stringstream buffer;

  string const& obj_name = weapon_info[static_cast<size_t>(item_subtype(item))].oi_name;

  if (item_count(item) == 1) {
    buffer << "A" << vowelstr(obj_name) << " " << obj_name;
  } else {
    buffer << item_count(item) << " " << obj_name << "s";
  }

  int dices;
  int sides;
  if (item_type(item) == AMMO || item_subtype(item) == BOW) {
    dices = item_throw_damage(item)->dices;
    sides = item_throw_damage(item)->sides;
  } else if (item_type(item) == WEAPON) {
    dices = item_damage(item)->dices;
    sides = item_damage(item)->sides;
  } else {
    error("Bad item type");
  }

  buffer << " (" << sides << "d" << dices << ")";

  if (item_is_known(item)) {
    buffer << " (";
    int p_hit = item_bonus_hit(item);
    if (p_hit >= 0) {
      buffer << "+";
    }
    buffer << p_hit << ",";

    int p_dmg = item_bonus_damage(item);
    if (p_dmg >= 0) {
      buffer << "+";
    }
    buffer << p_dmg << ")";
  }

  if (item_armor(item) != 0) {
    buffer << " [";
    int p_armor = item_armor(item);
    if (p_armor >= 0) {
      buffer << "+";
    }
    buffer << p_armor << "]";
  }

  if (!item->get_nickname().empty()) {
    buffer << " called " << item->get_nickname();
  }

  return buffer.str();
}

