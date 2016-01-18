#include <csignal>

#include "game.h"
#include "armor.h"
#include "daemons.h"
#include "food.h"
#include "io.h"
#include "level.h"
#include "misc.h"
#include "move.h"
#include "options.h"
#include "pack.h"
#include "player.h"
#include "potions.h"
#include "rings.h"
#include "rogue.h"
#include "score.h"
#include "scrolls.h"
#include "things.h"
#include "wand.h"
#include "weapons.h"
#include "wizard.h"

#include "command.h"
#include "command_private.h"

static bool
unknown_command(char ch)
{
  io_msg_unsaved("illegal command '%s'", unctrl(static_cast<chtype>(ch)));
  return false;
}

bool
command_stop(bool stop_fighting)
{
  player_stop_running();
  door_stop = false;
  running = false;
  player_alerted = true;

  if (stop_fighting)
    to_death = false;

  return false;
}

int
command()
{
  daemon_run_before();

  int num_moves = 1;
  num_moves += player_get_speed();

  while (num_moves-- > 0)
  {
    look(true);
    if (!running)
      door_stop = false;
    io_refresh_statusline();
    if (!(running && jump))
      refresh();


    if (player_turns_without_action && --player_turns_without_action == 0)
    {
      player_start_running();
      io_msg("you can move again");
    }

    Coordinate* player_pos = player_get_pos();
    move(player_pos->y, player_pos->x);

    if (!player_turns_without_action)
    {
      char ch;

      if (running || to_death)
        ch = runch;
      else
      {
        ch = io_readchar(false);
        io_msg_clear();
      }

      /* command_do returns 0 if player did something not in-game
       * (like changing options), thus recevies another turn */
      if (!command_do(ch))
        num_moves++;

      /* turn off flags if no longer needed */
      if (!running)
        door_stop = false;
    }

    if (!running)
      door_stop = false;
  }

  food_digest();
  daemon_run_after();
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
    case ',': return command_pick_up();
    case '/': return command_identify_character();
    case '>': return command_use_stairs(ch);
    case '<': return command_use_stairs(ch);
    case '?': return command_help();
    case '!': io_msg("Shell has been removed, use ^Z instead"); return false;
    case '^': return command_identify_trap();

    /* Lower case */
    case 'h': case 'j': case 'k': case 'l':
    case 'y': case 'u': case 'b': case 'n':
      return move_do(ch);
    case 'a': return command_attack(false);
    case 'c': return command_name_item();
    case 'd': return command_drop();
    case 'e': return command_eat();
    case 'i': return command_show_inventory();
    case 'o': return option();
    case 'q': return potion_quaff_something();
    case 'r': return scroll_read();
    case 's': return player_search();
    case 't': return command_throw();
    case 'w': return command_wield();
    case 'x': return weapon_wield_last_used();
    case 'z': return wand_zap();

    /* Upper case */
    case 'H': case 'J': case 'K': case 'L':
    case 'Y': case 'U': case 'B': case 'N':
      return command_run(ch, false);
    case 'A': return command_attack(true);
    case 'D': discovered(); return false;
    case 'I': return pack_print_equipment();
    case 'P': return ring_put_on();
    case 'Q': return command_quit();
    case 'R': return ring_take_off();
    case 'T': return command_take_off(EQUIPMENT_ARMOR);
    case 'W': return armor_command_wear();
    case 'Z': return command_rest();

    /* Ctrl case */
    case CTRL('H'): case CTRL('J'): case CTRL('K'): case CTRL('L'):
    case CTRL('Y'): case CTRL('U'): case CTRL('B'): case CTRL('N'):
      return command_run(UNCTRL(ch), true);
    case CTRL('P'): io_msg_last(); return false;
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
    case '|': io_msg("@ %d,%d", player_get_pos()->y, player_get_pos()->x); break;
    case 'C': wizard_create_item(); break;
    case '$': io_msg("inpack = %d", pack_count_items()); break;
    case CTRL('A'): Game::new_level(Game::current_level -1); break;
    case CTRL('Q'): Game::level->wizard_show_passages(); break;
    case CTRL('D'): Game::new_level(Game::current_level +1); break;
    case CTRL('E'): io_msg("food left: %d", food_nutrition_left()); break;
    case CTRL('F'): wizard_show_map(); break;
    case CTRL('I'): wizard_levels_and_gear(); break;
    case CTRL('T'): player_teleport(nullptr); break;
    case CTRL('W'): pack_identify_item(); break;
    case CTRL('X'): player_can_sense_monsters()
                    ? player_remove_sense_monsters(0)
                    : player_add_sense_monsters(true); break;
    case '*' : wizard_list_items(); break;

    case CTRL('~'):
     {
       Item* item = pack_get_item("charge", STICK);
       if (item != nullptr)
         item->o_charges = 10000;
     }
     break;

    default:
     return unknown_command(ch);
  }
  return false;
}

void
command_signal_endit(__attribute__((unused)) int sig)
{
  endwin();
  puts("Okay, bye bye!\n");
  exit(0);
}

void
command_signal_quit(__attribute__((unused)) int sig)
{
  int oy, ox;

  getyx(curscr, oy, ox);
  io_msg_clear();
  io_msg("really quit? ");

  if (io_readchar(true) == 'y')
  {
  /* Reset the signal in case we got here via an interrupt */
    signal(SIGINT, command_signal_leave);
    pack_evaluate();
    score_show_and_exit(pack_gold, 1, 0);
  }
  else
  {
    io_refresh_statusline();
    io_msg_clear();
    move(oy, ox);
    refresh();
    command_stop(true);
  }
}

void
command_signal_leave(__attribute__((unused)) int sig)
{
  static char buf[BUFSIZ];

  setbuf(stdout, buf);	/* throw away pending output */

  if (!isendwin())
  {
    mvcur(0, COLS - 1, LINES - 1, 0);
    endwin();
  }

  putchar('\n');
  exit(0);
}

