#include <string>
#include <list>

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

  switch (item->o_type)
  {
    case IO::Food:
      worth = 2 * item->o_count;
      break;

    case IO::Weapon: case IO::Ammo: {
      class Weapon* weapon = dynamic_cast<class Weapon*>(item);
      if (weapon == nullptr) {
        error("Could not cast weapon to Weapon class");
      }
      worth = Weapon::worth(static_cast<Weapon::Type>(item->o_which));
      worth *= 3 * (item->get_hit_plus() + item->get_damage_plus()) + item->o_count;
      weapon->set_identified();
    } break;

    case IO::Armor: {
      class Armor* armor = dynamic_cast<class Armor*>(item);
      if (armor == nullptr) {
        error("Could not cast armor to Armor class");
      }
      worth = Armor::value(static_cast<Armor::Type>(item->o_which));
      worth += (9 - item->get_armor()) * 100;
      worth += (10 * (Armor::ac(static_cast<Armor::Type>(item->o_which)) - item->get_armor()));
      armor->set_identified();
    } break;

    case IO::Scroll:
      {
        Scroll::Type scroll = static_cast<Scroll::Type>(item->o_which);
        worth = Scroll::worth(scroll);
        worth *= item->o_count;
        if (!Scroll::is_known(scroll))
          worth /= 2;
        Scroll::set_known(scroll);
      }
      break;

    case IO::Potion: {
      Potion::Type subtype = static_cast<Potion::Type>(item->o_which);
      worth = Potion::worth(subtype);
      worth *= item->o_count;
      if (!Potion::is_known(subtype)) {
        worth /= 2;
      }
      Potion::set_known(subtype);
    } break;

    case IO::Ring: {
      Ring::Type subtype = static_cast<Ring::Type>(item->o_which);
      worth = Ring::worth(subtype);
      if (subtype == Ring::Type::ADDSTR || subtype == Ring::Type::ADDDAM ||
          subtype == Ring::Type::PROTECT || subtype == Ring::Type::ADDHIT) {
        if (item->get_armor() > 0) {
          worth += item->get_armor() * 100;
        } else {
          worth = 10;
        }
      }
      if (Ring::is_known(subtype)) {
        worth /= 2;
      }
      Ring::set_known(subtype);
    } break;

    case IO::Wand: {
      Wand* wand = dynamic_cast<Wand* const>(item);
      if (wand == nullptr) {
        error("Could not cast wand to Wand class");
      }
      Wand::worth(static_cast<Wand::Type>(item->o_which));
      worth += 20 * wand->get_charges();
      if (!Wand::is_known(static_cast<Wand::Type>(item->o_which)))
        worth /= 2;
      Wand::set_known(static_cast<Wand::Type>(item->o_which));
    } break;

    case IO::Amulet:
      worth = 1000;
      break;

    default:
      io_debug_fatal("Unknown type: %c(%d)", item->o_type, item->o_type);
      break;
  }

  if (worth < 0)
    worth = 0;

  printw("%5d  %s\n", worth, item->get_description().c_str());

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
  if (obj->o_type == IO::Potion || obj->o_type == IO::Scroll ||
      obj->o_type == IO::Food || obj->o_type == IO::Ammo)
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
  if (pack_num_items(type, -1) < 1) {
    Game::io->message("You have no item to " + purpose);
    return nullptr;
  }

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

    char ch = io_readchar(true);
    Game::io->clear_message();
    touchwin(stdscr);

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
  WINDOW* equipscr = dupwin(stdscr);

  Coordinate orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  char sym = 'a';
  for (size_t i = 0; i < NEQUIPMENT; ++i) {
    Item* item = equipment.at(i);
    string item_description;

    if (item == nullptr) {
      item_description = "nothing";
    } else {
      item_description = item->get_description();
    }

    mvwprintw(equipscr, sym - 'a' + 1, 1, "%c) %s: %s",
        sym, equipment_pos_to_string(static_cast<Equipment>(i)).c_str(),
        item_description.c_str());
    sym++;
  }

  move(orig_pos.y, orig_pos.x);
  wrefresh(equipscr);
  delwin(equipscr);
  untouchwin(stdscr);
  return false;
}

bool Player::pack_print_inventory(int type) {
  WINDOW* invscr = dupwin(stdscr);

  Coordinate orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  int num_items = 0;
  /* Print out all items */
  for (Item const* list : pack) {
    if (!type || type == list->o_type) {
      /* Print out the item and move to next row */
      wmove(invscr, ++num_items, 1);
      wprintw(invscr, "%c) %s", list->o_packch, list->get_description().c_str());
    }
  }

  wmove(stdscr, orig_pos.y, orig_pos.x);
  wrefresh(invscr);
  delwin(invscr);
  untouchwin(stdscr);
  return num_items != 0;
}

size_t Player::pack_print_value() {
  size_t value = 0;

  clear();
  mvaddstr(0, 0, "Worth  Item  [Equipment]\n");
  for (size_t i = 0; i < static_cast<size_t>(NEQUIPMENT); ++i)
    value += pack_print_evaluate_item(equipment.at(i));

  addstr("\nWorth  Item  [Inventory]\n");
  for (Item* obj : pack) {
    value += pack_print_evaluate_item(obj);
  }

  printw("\n%5d  Gold Pieces          ", gold);
  refresh();
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

        case Ring1: doing = "wearing"; break;
        case Ring2: {
          doing = "wearing";
          if (item->o_which == Ring::AGGR) {
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
    Game::io->message("you can't. It appears to be cursed");
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

class Weapon* Player::equipped_weapon() {
  return dynamic_cast<class Weapon*>(equipment.at(Weapon));
}

class Armor* Player::equipped_armor() {
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

    } else if (obj->o_which == Ring::Type::SEARCH) {
      player->search();
    } else if (obj->o_which == Ring::Type::TELEPORT && os_rand_range(50) == 0) {
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

  Game::io->message(equipment.at(Weapon)->get_description());
  return true;
}
