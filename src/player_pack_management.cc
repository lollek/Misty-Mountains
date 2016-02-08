#include <list>

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
          Game::io->clear_message();
          return true;
        }
        break;
      }
    }
  }
}

bool Player::pack_show() {
  enum cur_win_t { INVENTORY, EQUIPMENT };
  cur_win_t current_window = INVENTORY;

  for (;;) {
    switch (current_window) {
      case INVENTORY: {
        pack_print_inventory(0);
        Game::io->message("Command? [(e)quip|(d)rop|ESC|SPACE]");
      } break;

      case EQUIPMENT: {
        pack_print_equipment();
        Game::io->message("Command? [(r)emove|ESC|SPACE]");
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
        if (ch == 'e') {
          if (pack_show_equip()) {
            return true;
          }
        }
      } break;

      case EQUIPMENT: {
        size_t position = static_cast<size_t>(ch - 'a');
        if (position < equipment.size()) {
          //return equipment->at(position).ptr;
          break;
        }
      } break;
    }
  }
}


