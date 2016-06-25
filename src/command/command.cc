#include <csignal>
#include <string>

#include "daemons.h"
#include "death.h"
#include "game.h"
#include "io.h"
#include "item/armor.h"
#include "item/food.h"
#include "item/potions.h"
#include "item/rings.h"
#include "item/scrolls.h"
#include "item/wand.h"
#include "item/weapons.h"
#include "level.h"
#include "misc.h"
#include "move.h"
#include "options.h"
#include "player.h"
#include "rogue.h"
#include "score.h"
#include "wizard.h"

#include "command.h"
#include "command_private.h"

using namespace std;

static bool unknown_command(char ch) {
  Game::io->message("illegal command '" + string(1, ch) + "'");
  player->set_not_running();
  return false;
}

bool command_stop(bool stop_fighting) {
  player->set_not_running();
  player_alerted = true;

  if (stop_fighting) {
    to_death = false;
  }
  return false;
}

int command() {
  Daemons::daemon_run_before();
  int num_moves{player->get_moves_this_round()};

  while (num_moves-- > 0) {
    Game::io->refresh();

    if (player_turns_without_action > 0 && --player_turns_without_action == 0) {
      Game::io->message("you can move again");
    }

    if (player_turns_without_action != 0) {
      continue;
    }

    char ch;
    if (player->is_running() || to_death) {
      ch = runch;

    } else {
      ch = Game::io->readchar(false);
      Game::io->clear_message();
    }

    // command_do returns 0 if player did something not in-game
    // (like changing options), thus recevies another turn
    if (!command_do(ch)) {
      num_moves++;
    }
  }

  player->digest_food();
  Daemons::daemon_run_after();
  return 0;
}

bool command_do(char ch) {
  switch (ch) {
    /* Funny symbols */
    case KEY_SPACE: return false;
    case KEY_ESCAPE: return command_stop(true);
    case '.': return true;
    case ',': return command_pick_up(true);
    case '>': return command_use_stairs(ch);
    case '<': return command_use_stairs(ch);
    case '?': return command_help();
    case '^': return command_identify_trap();
    case '{':
      return command_inscribe_item();

    /* Lower case */
    case 'h':
    case 'j':
    case 'k':
    case 'l':
    case 'y':
    case 'u':
    case 'b':
    case 'n': return move_do(ch, false);
    case 'a': return command_attack(false);
    case 'c': return command_close();
    case 'e': return command_eat();
    case 'o': return command_open();
    case 'q': return potion_quaff_something();
    case 'r': return command_read_scroll();
    case 's': player->search(); return true;
    case 't': return command_throw();
    case 'x': return player->pack_swap_weapons();
    case 'z':
      return wand_zap();

    /* Upper case */
    case 'H':
    case 'J':
    case 'K':
    case 'L':
    case 'Y':
    case 'U':
    case 'B':
    case 'N': return command_run(ch, false);
    case 'A': return command_attack(true);
    case 'D': return player->pack_show_drop(Player::INVENTORY);
    case 'E': return player->pack_show_equipment();
    case 'I': return player->pack_show_inventory();
    case 'O': return option();
    case 'S': return command_save();
    case 'Q': return command_quit();
    case 'Z':
      return command_rest();

    /* Ctrl case */
    case CTRL('H'):
    case CTRL('J'):
    case CTRL('K'):
    case CTRL('L'):
    case CTRL('Y'):
    case CTRL('U'):
    case CTRL('B'):
    case CTRL('N'): return command_run(ch, true);
    case CTRL('P'): Game::io->repeat_last_messages(); return false;
    case CTRL('R'): Game::io->force_redraw(); return false;
    case CTRL('Z'): command_shell(); return false;

    default: return wizard ? command_wizard_do(ch) : unknown_command(ch);
  }
}

bool command_wizard_do(char ch) {
  switch (ch) {
    case '_': raise(SIGINT); break;
    case '*': wizard_list_items(); break;
    case '&': wizard_create_item(); break;
    case CTRL('A'): Game::new_level(Game::current_level - 1); break;
    case CTRL('D'): Game::new_level(Game::current_level + 1); break;
    case CTRL('E'): {
      Game::io->message("food left: " +
                        to_string(player->get_nutrition_left()));
    } break;
    case CTRL('F'): wizard_show_map(); break;
    case CTRL('I'): wizard_levels_and_gear(); break;
    case CTRL('T'): player->teleport(nullptr); break;
    case CTRL('W'): player->pack_identify_item(); break;
    case CTRL('X'):
      player->can_sense_monsters() ? player->remove_sense_monsters()
                                   : player->set_sense_monsters();
      break;
    case CTRL('~'): {
      Wand* wand{
          static_cast<Wand*>(player->pack_find_item("charge", IO::Wand))};
      if (wand != nullptr) {
        wand->set_charges(10000);
      }
    } break;

    default: return unknown_command(ch);
  }
  return false;
}

void command_signal_endit(__attribute__((unused)) int sig) {
  puts("Okay, bye bye!\n");
  Game::exit();
}

void command_signal_quit(__attribute__((unused)) int sig) {
  Game::io->clear_message();
  Game::io->message("really quit?");

  if (Game::io->readchar(true) == 'y') {
    /* Reset the signal in case we got here via an interrupt */
    signal(SIGINT, command_signal_leave);
    score_show_and_exit(QUIT);

  } else if (player != nullptr) {
    Game::io->clear_message();
    Game::io->refresh();
    command_stop(true);
  }
}

void command_signal_leave(__attribute__((unused)) int sig) {
  static char buf[BUFSIZ];

  setbuf(stdout, buf); /* throw away pending output */

  putchar('\n');
  Game::exit();
}