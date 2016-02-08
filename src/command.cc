#include <csignal>
#include <string>

#include "game.h"
#include "armor.h"
#include "daemons.h"
#include "food.h"
#include "io.h"
#include "level.h"
#include "misc.h"
#include "move.h"
#include "options.h"
#include "player.h"
#include "potions.h"
#include "rings.h"
#include "rogue.h"
#include "score.h"
#include "scrolls.h"
#include "wand.h"
#include "weapons.h"
#include "wizard.h"

#include "command.h"
#include "command_private.h"

using namespace std;

static bool
unknown_command(char ch)
{
  Game::io->message("illegal command '" + string(1, ch) + "'");
  return false;
}

bool
command_stop(bool stop_fighting)
{
  door_stop = false;
  player->set_not_running();
  player_alerted = true;

  if (stop_fighting)
    to_death = false;

  return false;
}

int
command()
{
  Daemons::daemon_run_before();

  int num_moves = 1;
  num_moves += player->get_speed();

  while (num_moves-- > 0)
  {
    Game::io->refresh();

    if (!player->is_running())
      door_stop = false;

    if (player_turns_without_action > 0 &&
        --player_turns_without_action == 0) {
      Game::io->message("you can move again");
    }

    if (!player_turns_without_action) {

      char ch;

      if (player->is_running() || to_death)
        ch = runch;
      else
      {
        ch = io_readchar(false);
        Game::io->clear_message();
      }

      /* command_do returns 0 if player did something not in-game
       * (like changing options), thus recevies another turn */
      if (!command_do(ch))
        num_moves++;

      /* turn off flags if no longer needed */
      if (!player->is_running())
        door_stop = false;
    }


    if (!player->is_running())
      door_stop = false;
  }

  player->digest_food();
  Daemons::daemon_run_after();
  return 0;
}

bool
command_do(char ch)
{
  switch (ch)
  {
    /* Funny symbols */
    case KEY_SPACE: return false;
    case KEY_ESCAPE: return command_stop(true);
    case '.': return true;
    case ',': return command_pick_up(true);
    case '>': return command_use_stairs(ch);
    case '<': return command_use_stairs(ch);
    case '?': return command_help();
    case '^': return command_identify_trap();

    /* Lower case */
    case 'h': case 'j': case 'k': case 'l':
    case 'y': case 'u': case 'b': case 'n':
      return move_do(ch);
    case 'a': return command_attack(false);
    case 'c': return command_close();
    case 'd': return command_drop();
    case 'e': return command_eat();
    case 'i': return player->pack_show();
    case 'o': return command_open();
    case 'q': return potion_quaff_something();
    case 'r': return command_read_scroll();
    case 's': player->search(); return true;
    case 't': return command_throw();
    case 'z': return wand_zap();

    /* Upper case */
    case 'H': case 'J': case 'K': case 'L':
    case 'Y': case 'U': case 'B': case 'N':
      return command_run(ch, false);
    case 'A': return command_attack(true);
    case 'O': return option();
    case 'Q': return command_quit();
    case 'Z': return command_rest();

    /* Ctrl case */
    case CTRL('H'): case CTRL('J'): case CTRL('K'): case CTRL('L'):
    case CTRL('Y'): case CTRL('U'): case CTRL('B'): case CTRL('N'):
      return command_run(UNCTRL(ch), true);
    case CTRL('P'): Game::io->repeat_last_message(); return false;
    case CTRL('R'): clearok(curscr, true); wrefresh(curscr); return false;
    case CTRL('Z'): command_shell(); return false;

    default:
      return wizard
        ? command_wizard_do(ch)
        : unknown_command(ch);
  }
}

bool
command_wizard_do(char ch)
{
  switch (ch)
  {
    case '_': raise(SIGINT); break;
    case '|': {
      Coordinate c = player->get_position();
      Game::io->message("@ " + to_string(c.x) + "," + to_string(c.y));
    } break;
    case 'C': wizard_create_item(); break;
    case '*' : wizard_list_items(); break;
    case CTRL('A'): Game::new_level(Game::current_level -1); break;
    case CTRL('Q'): Game::level->wizard_show_passages(); break;
    case CTRL('D'): Game::new_level(Game::current_level +1); break;
    case CTRL('E'): {
      Game::io->message("food left: " + to_string(player->get_nutrition_left()));
    } break;
    case CTRL('F'): wizard_show_map(); break;
    case CTRL('I'): wizard_levels_and_gear(); break;
    case CTRL('T'): player->teleport(nullptr); break;
    case CTRL('W'): player->pack_identify_item(); break;
    case CTRL('X'): player->can_sense_monsters()
                    ? player->remove_sense_monsters()
                    : player->set_sense_monsters(); break;
    case CTRL('~'): {
       Wand* wand = static_cast<Wand*>(player->pack_find_item("charge", IO::Wand));
       if (wand != nullptr) {
         wand->set_charges(10000);
       }
     } break;

    default:
     return unknown_command(ch);
  }
  return false;
}

void
command_signal_endit(__attribute__((unused)) int sig)
{
  puts("Okay, bye bye!\n");
  Game::exit();
}

void
command_signal_quit(__attribute__((unused)) int sig)
{
  int oy, ox;

  getyx(curscr, oy, ox);
  Game::io->clear_message();
  Game::io->message("really quit?");

  if (io_readchar(true) == 'y')
  {
  /* Reset the signal in case we got here via an interrupt */
    signal(SIGINT, command_signal_leave);
    player->pack_print_value();
    score_show_and_exit(player->get_gold(), 1, 0);
  }
  else
  {
    Game::io->clear_message();
    move(oy, ox);
    Game::io->refresh();
    command_stop(true);
  }
}

void
command_signal_leave(__attribute__((unused)) int sig)
{
  static char buf[BUFSIZ];

  setbuf(stdout, buf);	/* throw away pending output */

  putchar('\n');
  Game::exit();
}

