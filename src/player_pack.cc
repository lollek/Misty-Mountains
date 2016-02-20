#include <string>
#include <list>
#include <algorithm>
#include <iomanip>
#include <sstream>

#include "error_handling.h"
#include "coordinate.h"
#include "game.h"
#include "scrolls.h"
#include "io.h"
#include "armor.h"
#include "potions.h"
#include "rings.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "weapons.h"
#include "wand.h"
#include "monster.h"
#include "os.h"
#include "level_rooms.h"
#include "options.h"
#include "rogue.h"
#include "item.h"
#include "gold.h"

#include "player.h"

using namespace std;

static size_t
pack_print_evaluate_item(Item* item)
{
  int worth = 0;
  if (item == nullptr)
    return 0;
  worth = item->get_value();
  item->set_identified();

  stringstream ss;
  ss << setw(5) << setfill(' ') << worth << setw(0) << "  "
     << item->get_description() << "\n";
  Game::io->print_string(ss.str());

  return static_cast<unsigned>(worth);
}




bool Player::pack_add(Item* obj, bool silent, bool from_floor) {
  /* Either obj is an item or we try to take something from the floor */
  if (obj == nullptr) {
    obj = Game::level->get_item(player->get_position());
    if (obj == nullptr) {
      error("Item not found on floor");
    }
    from_floor = true;
  }

  /* See if we can stack it with something else in the pack */
  bool is_picked_up = false;
  if (obj->is_stackable())
    for (Item* ptr : pack) {
      if (ptr->o_type == obj->o_type && ptr->o_which == obj->o_which &&
          ptr->get_hit_plus() == obj->get_hit_plus() &&
          ptr->get_damage_plus() == obj->get_damage_plus())
      {
        if (from_floor)
          Game::level->items.remove(obj);
        ptr->o_count += obj->o_count;
        ptr->set_position(obj->get_position());
        delete obj;
        obj = ptr;
        is_picked_up = true;
        break;
      }
    }

  /* If we cannot stack it, we need to have available space in the pack */
  if (!is_picked_up && pack.size() == pack_size())
  {
    Game::io->message("there's no room in your pack");
    if (from_floor) {
      Game::io->message("moved onto " + obj->get_description());
    }
    return false;
  }

  /* Otherwise, just insert it */
  if (!is_picked_up)
  {
    if (from_floor)
      Game::level->items.remove(obj);
    pack.push_back(obj);
    for (size_t i = 0; i < pack_size(); ++i) {
      char packch = static_cast<char>(i) + 'a';
      auto results = find_if(pack.begin(), pack.end(),
        [packch] (Item* it) {
        return it->o_packch == packch;
      });

      if (results == pack.end()) {
        obj->o_packch = packch;
        break;
      }
    }
  }

  /* Notify the user */
  if (!silent) {
    Game::io->message("you now have " + obj->get_description() +
                      " (" + string(1, obj->o_packch) + ")");
  }
  return true;
}

Item* Player::pack_remove(Item* obj, bool newobj, bool all) {
  Item* return_value = obj;

  // If equipped, remove it
  for (size_t i = 0; i < equipment.size(); ++i) {
    if (equipment.at(i) == obj) {
      equipment.at(i) = nullptr;
    }
  }

  /* If there are several, we need to alloate a new item to hold it */
  if (obj->o_count > 1 && !all) {
    obj->o_count--;
    if (newobj) {
      return_value = obj->clone();
      return_value->o_count = 1;
    }

  /* Only one item? Just pop and return it */
  } else {
    pack.remove(obj);
  }
  return return_value;
}

Item* Player::pack_find_random_item() {
  if (pack.empty()) {
    return nullptr;
  }

  auto it = pack.begin();
  advance(it, static_cast<long>(os_rand_range(pack.size())));
  return *it;
}

class Weapon* Player::pack_find_ammo(Weapon::AmmoType ammo_type) {
  auto results = find_if(pack.begin(), pack.end(),
      [ammo_type] (Item* i) {
    class Weapon* w = dynamic_cast<class Weapon*>(i);
    return w != nullptr && w->get_ammo_type() == ammo_type;
  });

  return results == pack.end() ? nullptr : dynamic_cast<class Weapon*>(*results);
}

Item* Player::pack_find_item(string const& purpose, int type) {
  enum cur_win_t { INVENTORY, EQUIPMENT };
  cur_win_t current_window = INVENTORY;

  for (;;) {
    switch (current_window) {
      case INVENTORY: {
        pack_print_inventory(type);
        Game::io->message("Inventory [" + purpose + "? E ESC]");
      } break;

      case EQUIPMENT: {
        pack_print_equipment();
        Game::io->message("Equipment [" + purpose + "? I ESC]");
      } break;
    }

    char ch = Game::io->readchar(true);
    Game::io->clear_message();

    if (ch == KEY_ESCAPE) {
      Game::io->clear_message();
      return nullptr;
    }

    switch (current_window) {
      case INVENTORY: {
        if (ch == 'E') {
          current_window = EQUIPMENT;
          continue;
        }

        for (Item* obj : pack) {
          if (obj->o_packch == ch) {
            return obj;
          }
        }
      } break;

      case EQUIPMENT: {
        if (ch == 'I') {
          current_window = INVENTORY;
          continue;
        }
        size_t position = static_cast<size_t>(ch - 'a');
        if (position < equipment.size() && equipment.at(position) != nullptr) {
          return equipment.at(position);
        }
      } break;
    }
  }
}

size_t Player::pack_num_items(int type, int subtype) {
  size_t num = 0;

  for (Item const* list : pack) {
    if (!type || (type == list->o_type && (list->o_which == subtype || subtype == -1))) {
      ++num;
    }
  }
  return num;
}

string Player::equipment_pos_to_string(Equipment pos) {
  switch (pos) {
    case Armor:         return "Armor";
    case Weapon:        return "Weapon";
    case BackupWeapon:  return "Backup Weapon";
    case Ring1:
    case Ring2:         return "Ring";
    case NEQUIPMENT: error("equipment out of range");
  }
}

bool Player::pack_contains_amulet() {
  return find_if(pack.cbegin(), pack.cend(), [] (Item const* ptr) {
    return ptr->o_type == IO::Amulet;
  }) != pack.cend();
}

bool Player::pack_contains(Item const* item) {
  return find(pack.cbegin(), pack.cend(), item) != pack.cend();
}

bool Player::pack_print_equipment() {
  char sym = 'a';
  for (size_t i = 0; i < NEQUIPMENT; ++i) {
    Item* item = equipment.at(i);
    string item_description;

    if (item == nullptr) {
      item_description = "nothing";
    } else {
      item_description = item->get_description();
    }

    stringstream ss;
    ss << sym << ") " << equipment_pos_to_string(static_cast<Equipment>(i))
       << ": " << item_description;
    Game::io->print_string(IO::message_x + 1, sym - 'a' + 1, ss.str());
    sym++;
  }

  Game::io->force_redraw();
  return false;
}

bool Player::pack_print_inventory(int type) {
  int num_items = 0;
  /* Print out all items */
  for (Item const* list : pack) {
    if (!type || type == list->o_type) {
      stringstream ss;
      ss << list->o_packch << ") " << list->get_description();
      Game::io->print_string(IO::message_x + 1, ++num_items, ss.str());
    }
  }

  return num_items != 0;
}

size_t Player::pack_print_value() {
  size_t value = 0;

  Game::io->clear_screen();
  Game::io->print_string(0, 0, "Worth  Item  [Equipment]\n");
  for (size_t i = 0; i < static_cast<size_t>(NEQUIPMENT); ++i) {
    value += pack_print_evaluate_item(equipment.at(i));
  }

  Game::io->print_string("\nWorth  Item  [Inventory]\n");
  for (Item* obj : pack) {
    value += pack_print_evaluate_item(obj);
  }

  stringstream ss;
  ss << "\n" << setw(5) << setfill(' ') << gold << "  Gold Pieces          ";
  Game::io->print_string(ss.str());
  Game::io->move_pointer(0, IO::screen_height -1);
  Game::io->force_redraw();
  return value;
}

bool Player::pack_equip(Item* item, bool silent) {
  Equipment position;

  switch(item->o_type) {

    case IO::Armor: {
      position = Armor;
    } break;

    default: case IO::Weapon: case IO::Ammo: {
      position = Weapon;
    } break;

    case IO::Ring: {
      position = Ring1;
    } break;
  }


  for (;;) {
    if (equipment.at(static_cast<size_t>(position)) == nullptr) {

      pack_remove(item, false, true);
      equipment.at(static_cast<size_t>(position)) = item;

      string doing;
      switch (position) {
        case Armor: {
          doing = "wearing";
          waste_time(1);
        } break;

        case Ring1:
        case Ring2: {
          doing = "wearing";
          if (item->o_which == Ring::AggravateMonsters) {
            monster_aggravate_all();
          }
        } break;

        case Weapon:        doing = "wielding"; break;
        case BackupWeapon:  doing = "wielding"; break;

        case NEQUIPMENT: error("NEQUIPMENT received");
      }

      if (!silent) {
        Game::io->message("now " + doing + " " + item->get_description());
      }
      return true;

    } else if (position == Ring1) {
      position = Ring2;
      continue;

    } else if (position == Armor) {
      if (pack_unequip(position, true)) {
        continue;
      } else {
        return false;
      }

    } else {
      Game::io->message("the slot is already in use");
      return false;
    }
  }
}

bool Player::pack_unequip(Equipment pos, bool silent_on_success) {
  string doing;
  switch (pos) {
    case BackupWeapon: case Armor: case Ring1: case Ring2: {
      doing = "wearing";
    } break;

    case Weapon: {
      doing = "wielding";
    } break;

    case NEQUIPMENT: error("NEQUIPMENT");
  }

  Item* obj = equipment.at(pos);
  if (obj == nullptr) {
    Game::io->message("not " + doing + " anything!");
    return false;
  }

  if (obj->is_cursed()) {
    Game::io->message("you can't unequip it. It appears to be cursed");
    obj->set_identified();
    return false;
  }

  equipment.at(pos) = nullptr;

  /* Waste time if armor - since they take a while */
  if (pos == Armor) {
    player->waste_time(1);
  }

  if (!pack_add(obj, true, false)) {
    Game::level->items.push_back(obj);
    obj->set_position(player->get_position());
    Game::io->message("dropped " + obj->get_description());

  } else if (!silent_on_success) {
    Game::io->message("no longer " + doing + " " + obj->get_description());
  }

  return true;
}

Item* Player::pack_find_item(int type, int subtype)
{
  auto results = find_if(pack.begin(), pack.end(),
      [type, subtype] (Item *i) {
    return i->o_type == type && i->o_which == subtype;
  });

  return results == pack.end() ? nullptr : *results;
}

class Weapon* Player::equipped_weapon() const {
  return dynamic_cast<class Weapon*>(equipment.at(Weapon));
}

class Armor* Player::equipped_armor() const {
  return dynamic_cast<class Armor*>(equipment.at(Armor));
}

void Player::give_gold(int amount) {
  gold += amount;
}

int Player::get_gold() {
  return gold;
}

void Player::pack_identify_item() {
  Item* obj = pack_find_item("identify", 0);
  if (obj == nullptr) {
    return;
  }

  obj->set_identified();
  Game::io->message(obj->get_description());
}


void Player::equipment_run_abilities() {
  for (Equipment position : all_rings()) {
    Item* obj = equipment.at(position);
    if (obj == nullptr) {
      continue;

    } else if (obj->o_which == Ring::Searching) {
      player->search();
    } else if (obj->o_which == Ring::Teleportation && os_rand_range(50) == 0) {
      player->teleport(nullptr);
    }
  }
}

size_t Player::equipment_size() {
  return NEQUIPMENT;
}

size_t Player::pack_size() {
  return 22;
}

bool Player::pack_swap_weapons() {
  Item* main_weapon = equipment.at(Weapon);
  if (main_weapon != nullptr && main_weapon->is_cursed()) {
    Game::io->message("you can't. It appears to be cursed");
    return true;
  }

  Item *backup_weapon = equipment.at(BackupWeapon);

  equipment.at(Weapon) = backup_weapon;
  equipment.at(BackupWeapon) = main_weapon;

  if (equipped_weapon() != nullptr) {
    Game::io->message(equipped_weapon()->get_description());
  } else {
    Game::io->message("no weapon");
  }
  return true;
}

int Player::pack_get_ring_modifier(Ring::Type ring_type) {
  int return_value = 0;
  for (Equipment position : all_rings()) {
    Ring* obj = dynamic_cast<Ring*>(equipment.at(position));
    if (obj == nullptr) {
      continue;

    } else if (obj->o_which == ring_type) {
      return_value += obj->get_armor();
    }
  }

  return return_value;
}
