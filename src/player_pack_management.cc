#include <list>
#include <functional>

#include "game.h"

#include "player.h"

using namespace std;

bool Player::pack_show_equip() {
  for (;;) {
    pack_print_inventory(0);
    Game::io->message("Equip what item? (ESC to abort)");

    char ch = io_readchar(true);
    Game::io->clear_message();

    if (ch == KEY_ESCAPE) {
      return false;
    }

    for (Item* obj : pack) {
      if (obj->o_packch == ch) {
        if (player->pack_equip(obj, false)) {
          return true;
        }
        break;
      }
    }
  }
}

bool Player::pack_show_drop() {
  for (;;) {
    pack_print_inventory(0);
    Game::io->message("Drop what item? (ESC to abort)");

    char ch = io_readchar(true);
    Game::io->clear_message();

    if (ch == KEY_ESCAPE) {
      return false;
    }

    for (Item* obj : pack) {
      if (obj->o_packch == ch) {
        bool drop_all = false;
        if (obj->o_count > 1) {
          Game::io->message("Drop all? (y/N) ");

          ch = io_readchar(true);
          if (ch == KEY_ESCAPE) {
            return false;
          } else {
            drop_all = ch == 'y';
          }
          Game::io->clear_message();
        }

        obj = player->pack_remove(obj, true, drop_all);
        Game::level->items.push_back(obj);
        obj->set_position(player->get_position());
        Game::io->message("dropped " + obj->get_description());
        return true;
      }
    }
  }
}

bool Player::pack_show_remove() {
  for (;;) {
    pack_print_equipment();
    Game::io->message("remove what item? (ESC to abort)");

    char ch = io_readchar(true);
    Game::io->clear_message();

    if (ch == KEY_ESCAPE) {
      return false;
    }

    size_t position = static_cast<size_t>(ch - 'a');
    if (position < equipment.size()) {
      if (equipment.at(position) == nullptr) {
        return false;
      }

      pack_unequip(static_cast<Equipment>(position), false);
      return equipment.at(position) == nullptr;
    }
  }
}


bool Player::pack_show_inventory() {
  return pack_show(0);
}

bool Player::pack_show_equipment() {
  return pack_show(1);
}


bool Player::pack_show(int mode) {
  enum cur_win_t { INVENTORY, EQUIPMENT };
  cur_win_t current_window = static_cast<cur_win_t>(mode);

  for (;;) {
    switch (current_window) {
      case INVENTORY: {
        pack_print_inventory(0);
        Game::io->message("Inventory [(e)quip|(d)rop|ESC|SPACE]");
      } break;

      case EQUIPMENT: {
        pack_print_equipment();
        Game::io->message("Equipment [(r)emove|ESC|SPACE]");
      } break;
    }

    char ch = io_readchar(true);
    Game::io->clear_message();
    touchwin(stdscr);

    if (ch == KEY_SPACE) {
      if (current_window == INVENTORY) {
        current_window = EQUIPMENT;
      } else if (current_window == EQUIPMENT) {
        current_window = INVENTORY;
      }
      continue;

    } else if (ch == KEY_ESCAPE) {
      Game::io->clear_message();
      return false;
    }

    switch (current_window) {
      case INVENTORY: {
        switch (ch) {
          case 'e':  {
            if (pack_show_equip()) {
              touchwin(stdscr);
              return true;
            }
          } break;

          case 'd': {
            if (pack_show_drop()) {
              touchwin(stdscr);
              return true;
            }
          } break;
        }


      } break;

      case EQUIPMENT: {
        switch (ch) {
          case 'r': {
            if (pack_show_remove()) {
              touchwin(stdscr);
              return true;
            }
          } break;
        }
      } break;
    }
  }
}


