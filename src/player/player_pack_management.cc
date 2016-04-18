#include <functional>
#include <list>

#include "game.h"

#include "player.h"

using namespace std;

bool Player::pack_show_equip() {
  for (;;) {
    pack_print_inventory(0);
    Game::io->message("Equip what item? (ESC to abort)", true);

    char ch{Game::io->readchar(true)};
    Game::io->clear_message();

    if (ch == KEY_ESCAPE) { return false; }

    for (Item* obj : pack) {
      if (obj->o_packch == ch) {
        if (player->pack_equip(obj, false)) { return true; }
        break;
      }
    }
  }
}

bool Player::pack_show_drop(Window window) {
  for (;;) {
    switch (window) {
      case INVENTORY: pack_print_inventory(0); break;
      case EQUIPMENT: pack_print_equipment(); break;
    }
    Game::io->message("Drop what item? (ESC to abort)", true);

    char ch{Game::io->readchar(true)};
    Game::io->clear_message();

    if (ch == KEY_ESCAPE) { return false; }

    Item* obj{nullptr};
    switch (window) {
      case INVENTORY: {
        for (Item* i : pack) {
          if (i->o_packch == ch) {
            obj = i;
            break;
          }
        }
      } break;

      case EQUIPMENT: {
        size_t const position{static_cast<size_t>(ch - 'a')};
        if (position < equipment.size()) { obj = equipment.at(position); }
      } break;
    }

    if (obj == nullptr) {
      Game::io->message("No item at position " + string(1, ch));
      return false;
    }

    bool drop_all{false};
    if (obj->o_count > 1) {
      Game::io->message("Drop all? (y/N) ");

      ch = Game::io->readchar(true);
      if (ch == KEY_ESCAPE) {
        return false;
      } else {
        drop_all = ch == 'y';
      }
      Game::io->clear_message();
    }

    if (window == EQUIPMENT &&
        !pack_unequip(static_cast<Equipment>(ch - 'a'), true)) {
      return true;
    }

    obj = player->pack_remove(obj, true, drop_all);
    Game::level->items.push_back(obj);
    obj->set_position(player->get_position());
    Game::io->message("dropped " + obj->get_description());
    return true;
  }
}

bool Player::pack_show_remove() {
  for (;;) {
    pack_print_equipment();
    Game::io->message("remove what item? (ESC to abort)", true);

    char ch{Game::io->readchar(true)};
    Game::io->clear_message();

    if (ch == KEY_ESCAPE) { return false; }

    size_t const position{static_cast<size_t>(ch - 'a')};
    if (position < equipment.size()) {
      if (equipment.at(position) == nullptr) {
        Game::io->message("No item at position " + string(1, ch));
        return false;
      }

      pack_unequip(static_cast<Equipment>(position), false);
      return equipment.at(position) == nullptr;
    }
  }
}

bool Player::pack_show_inventory() { return pack_show(INVENTORY); }

bool Player::pack_show_equipment() { return pack_show(EQUIPMENT); }

bool Player::pack_show(Window current_window) {
  for (;;) {
    Game::io->refresh();
    switch (current_window) {
      case INVENTORY: {
        pack_print_inventory(0);
        Game::io->message("Inventory [e d E ESC]", true);
      } break;

      case EQUIPMENT: {
        pack_print_equipment();
        Game::io->message("Equipment [r d I ESC]", true);
      } break;
    }

    char ch{Game::io->readchar(true)};
    Game::io->clear_message();

    if (ch == KEY_ESCAPE) {
      Game::io->clear_message();
      return false;
    }

    switch (current_window) {
      case INVENTORY: {
        switch (ch) {
          case 'E': current_window = EQUIPMENT; break;
          case 'e': {
            if (pack_show_equip()) { return true; }
          } break;

          case 'd': {
            if (pack_show_drop(current_window)) { return true; }
          } break;
        }

      } break;

      case EQUIPMENT: {
        switch (ch) {
          case 'I': current_window = INVENTORY; break;
          case 'd': {
            if (pack_show_drop(current_window)) { return true; }
          } break;

          case 'r': {
            if (pack_show_remove()) { return true; }
          } break;
        }
      } break;
    }
  }
}
