#include <cmath>
#include <sstream>

#include "io.h"
#include "food.h"
#include "player.h"
#include "game.h"
#include "weapons.h"
#include "armor.h"
#include "scrolls.h"

#include "shop.h"

using namespace std;

Shop::~Shop() {
  for (Item* item : inventory) {
    delete item;
  }

  for (Item* item : limited_inventory) {
    delete item;
  }
}

Shop::Shop() {
  inventory.push_back(new class Food(Food::IronRation));
  inventory.push_back(new class Weapon(Weapon::Sling, false));
  inventory.push_back(new class Weapon(Weapon::Arrow, false));
  inventory.push_back(new class Weapon(Weapon::Dagger, false));
  inventory.push_back(new class Weapon(Weapon::Club, false));
  inventory.push_back(new class Weapon(Weapon::Shortbow, false));
  inventory.push_back(new class Armor(Armor::Robe, false));
  inventory.push_back(new class Armor(Armor::Softleatherarmor, false));
  inventory.push_back(new class Scroll(Scroll::ID));

  for (Item* item : inventory) {
    item->set_identified();
  }
}

int Shop::buy_value(Item const* item) const {
  return static_cast<int>(lround(item->get_value() * 1.1));
}

int Shop::sell_value(Item const* item) const {
  int value;
  if (item->is_identified()) {
    value = item->get_value();
  } else {
    value = item->get_base_value() * item->o_count;
  }

  // Cheap items
  if (10 > value && value > 0) {
    value /= 2;
    if (value <= 0) {
      value = 1;
    }

  // Normal and expensive items
  } else {
    value /= 10;
  }
  return value;
}

void Shop::print() const {
  char sym = 'a';

  stringstream ss;
  ss << "You have " << player->get_gold() << " gold";
  Game::io->print_string(IO::map_start_x + 1, 1, ss.str());
  Game::io->print_string(IO::map_start_x + 4, 3, "Item");
  Game::io->print_string(IO::map_start_x + 60, 3, "Price");

  // Unlimited inventory
  for (int i = 0; i < static_cast<int>(inventory.size()); ++i) {
    Item const* item = inventory.at(static_cast<size_t>(i));
    ss.clear();
    ss.str(string());
    ss << sym << ") " << item->get_description();
    Game::io->print_string(IO::map_start_x + 1, i + 4, ss.str());
    Game::io->print_string(IO::map_start_x + 60, i + 4, to_string(buy_value(item)));
    ++sym;

  }

  // Buyback
  for (Item* item : limited_inventory) {
    ss.clear();
    ss.str(string());
    ss << sym << ") " << item->get_description();
    Game::io->print_string(IO::map_start_x + 1, 4 + sym - 'a', ss.str());
    Game::io->print_string(IO::map_start_x + 60, 4 + sym - 'a', to_string(buy_value(item)));

    // Make sure we don't have too many items
    if (sym == 'a' + max_items_per_page) {
      break;
    }
    ++sym;
  }

}

int Shop::sell() {
  Game::io->clear_screen();

  Item* obj = player->pack_find_item("sell", 0);
  if (obj == nullptr) {
    return 0;
  }

  bool sell_all = false;
  if (obj->o_count > 1) {
    Game::io->message("Sell all? (y/N)");

    char ch = Game::io->readchar(true);
    if (ch == KEY_ESCAPE) {
      return sell();
    } else {
      sell_all = ch == 'y';
    }
    Game::io->clear_message();
  }

  int value = sell_value(obj);
  if (value <= 0) {
    Game::io->message("the shopkeeper is not interested in buying that");
    return sell();
  }

  int real_count = obj->o_count;
  if (!sell_all) {
    obj->o_count = 1;
    value = sell_value(obj);
  }

  stringstream os;
  os << "sell " << obj->get_description() << " for " << value << "? (y/N)";
  Game::io->message(os.str());

  obj->o_count = real_count;

  if (Game::io->readchar(true) != 'y') {
    return sell();
  }

  Game::io->clear_message();
  obj = player->pack_remove(obj, true, sell_all);
  obj->set_identified();
  player->give_gold(value);


  // Try to stack it
  for (Item *ptr : inventory) {
    if (ptr->o_type == obj->o_type &&
        ptr->o_which == obj->o_which &&
        ptr->get_hit_plus() == obj->get_hit_plus() &&
        ptr->get_damage_plus() == obj->get_damage_plus()) {
      delete obj;
      obj = nullptr;
      break;
    }
  }

  if (obj != nullptr && obj->is_stackable()) {
    for (Item *ptr : limited_inventory) {
      if (ptr->o_type == obj->o_type &&
          ptr->o_which == obj->o_which &&
          ptr->get_hit_plus() == obj->get_hit_plus() &&
          ptr->get_damage_plus() == obj->get_damage_plus()) {
        ptr->o_count += obj->o_count;
        delete obj;
        obj = nullptr;
        break;
      }
    }
  }

  if (obj != nullptr) {
    limited_inventory.push_back(obj);
  }
  return sell();
}

void Shop::enter() {
  for (;;) {
    Game::io->clear_screen();
    print();
    Game::io->clear_message();
    Game::io->message("Which item do you want to buy? [S to sell, ESC to return]", true);
    char ch = Game::io->readchar(true);
    Game::io->clear_message();

    if (ch == KEY_ESCAPE) {
      return;
    } else if (ch == 'S') {
      sell();
      continue;
    }


    size_t item_pos = static_cast<size_t>(ch - 'a');
    if (item_pos > max_items_per_page) {
      continue;
    }

    Item* item_to_buy = nullptr;
    bool limited_item = false;
    if (item_pos < inventory.size()) {
      item_to_buy = inventory.at(item_pos);

    } else if (item_pos - inventory.size() < limited_inventory.size()) {
      auto it = limited_inventory.begin();
      advance(it, static_cast<long>(item_pos - inventory.size()));
      item_to_buy = *it;
      limited_item = true;

    } else {
      continue;
    }

    int value = buy_value(item_to_buy);
    if (player->get_gold() < value) {
      Game::io->message("you cannot afford it");
      continue;
    }

    player->give_gold(-value);
    if (limited_item) {
      limited_inventory.remove(item_to_buy);
      player->pack_add(item_to_buy, false, false);
    } else {
      player->pack_add(item_to_buy->clone(), false, false);
    }
  }
}
