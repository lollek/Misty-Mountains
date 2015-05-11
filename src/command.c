/*
 * Read and execute the user commands
 *
 * @(#)command.c	4.73 (Berkeley) 08/06/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#include "potions.h"
#include "scrolls.h"
#include "options.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "daemons.h"
#include "list.h"
#include "move.h"
#include "level.h"
#include "passages.h"
#include "rings.h"
#include "save.h"
#include "misc.h"
#include "player.h"
#include "wizard.h"
#include "weapons.h"
#include "wand.h"
#include "things.h"
#include "rip.h"
#include "rogue.h"

#include "command.h"
#include "command_private.h"

void
command_stop(bool stop_fighting)
{
  player_stop_running();
  door_stop = false;
  again = false;
  running = false;

  if (stop_fighting)
    to_death = false;
}

int
command(void)
{
  static char ch;
  int num_moves;

  /* Let the daemons start up */
  daemon_run_all(BEFORE);
  daemon_run_fuses(BEFORE);

  for (num_moves = 1; num_moves > 0; --num_moves)
  {
    coord *player_pos = player_get_pos();

    if (player_is_hasted())
      num_moves++;

    /* TODO: Try to remove this */
    if (has_hit)
    {
      endmsg();
      has_hit = false;
    }

    again = false;
    look(true);
    if (!running)
      door_stop = false;
    status();
    move(player_pos->y, player_pos->x);
    if (!((running) && jump))
      refresh();
    take = 0;
    after = true;


    if (no_command)
    {
      ch = '.';
      if (--no_command == 0)
      {
        player_start_running();
        msg("you can move again");
      }
    }

    if (!no_command)
    {
      if (running || to_death)
        ch = runch;
      else
      {
        ch = readchar();
        move_on = false;
        if (mpos != 0)
          msg("");
      }

      if (ch != 'a' && ch != KEY_ESCAPE && !(running || to_death))
      {
        l_last_comm = last_comm;
        l_last_dir = last_dir;
        l_last_pick = last_pick;
        last_comm = ch;
        last_dir = '\0';
        last_pick = NULL;
      }
      after = command_do(ch);

      /* turn off flags if no longer needed */
      if (!running)
        door_stop = false;
    }

    /* If he ran into something to take, let him pick it up.  */
    if (take != 0)
      pack_pick_up(take);
    if (!running)
      door_stop = false;
    if (!after)
      num_moves++;
  }

  daemon_run_all(AFTER);
  daemon_run_fuses(AFTER);

  /* Do ring abilities */
  {
    int i;
    for (i = 0; i < RING_SLOTS_SIZE; ++i)
    {
      THING *obj = pack_equipped_item(ring_slots[i]);
      if (obj == NULL)
        continue;
      else if (obj->o_which == R_SEARCH)
        command_search();
      else if (obj->o_which == R_TELEPORT && rnd(50) == 0)
        player_teleport(NULL);
    }
  }

  return 0; /* Restart from top */
}

bool
command_do(char ch)
{
  switch (ch)
  {
    /* Funny symbols */
    case KEY_SPACE: return false;
    case KEY_ESCAPE: command_stop(true); return false;
    case '.': return true;
    case ',': return command_pick_up();
    case '/': return command_identify_character();
    case '>': return command_use_stairs(ch);
    case '<': return command_use_stairs(ch);
    case '?': return command_help();
    case '!': msg("Shell has been removed, use ^Z instead"); return false;
    case '^': return command_identify_trap();
    case '+': return command_toggle_wizard();

    /* Lower case */
    case 'h': case 'j': case 'k': case 'l':
    case 'y': case 'u': case 'b': case 'n':
      return move_do(ch);
    case 'a': return command_again();
    case 'c': return command_name_item();
    case 'd': return drop();
    case 'e': return eat();
    case 'f': return command_attack(false);
    case 'i': return command_show_inventory();
    case 'm': move_on = true; return get_dir() ? command_do(dir_ch) : false;
    case 'o': return option();
    case 'q': return potion_quaff_something();
    case 'r': return read_scroll();
    case 's': return command_search();
    case 't': return command_throw();
    case 'w': return command_wield();
    case 'x': return last_weapon();
    case 'z': return wand_zap();

    /* Upper case */
    case 'H': case 'J': case 'K': case 'L':
    case 'Y': case 'U': case 'B': case 'N':
      return move_do_run(ch, false);
    case 'D': discovered(); return false;
    case 'F': return command_attack(true);
    case 'I': return pack_print_equipment();
    case 'P': return ring_put_on();
    case 'R': return ring_take_off();
    case 'S': return save_game();
    case 'T': return command_take_off(EQUIPMENT_ARMOR);
    case 'W': return armor_command_wear();
    case 'Q': return command_quit();

    /* Ctrl case */
    case CTRL('H'): case CTRL('J'): case CTRL('K'): case CTRL('L'):
    case CTRL('Y'): case CTRL('U'): case CTRL('B'): case CTRL('N'):
      return move_do_run(UNCTRL(ch), true);
    case CTRL('P'): msg(huh); return false;
    case CTRL('R'): clearok(curscr, true); wrefresh(curscr); return false;
    case CTRL('Z'): command_shell(); return false;

    default:
      if (wizard)
        return command_wizard_do(ch);
      else
      {
        char buf[MAXSTR];
        /* Message the player, but don't save it to ^P */
        strcpy(buf, huh);
        msg("illegal command '%s'", unctrl(ch));
        strcpy(huh, buf);
        return false;
      }
  }
}

bool
command_wizard_do(char ch)
{
  after = false;

  switch (ch)
  {
    case '|': msg("@ %d,%d", player_get_pos()->y, player_get_pos()->x); break;
    case 'C': create_obj(); break;
    case '$': msg("inpack = %d", pack_count_items()); break;
    case CTRL('A'): level--; level_new(); break;
    case CTRL('C'): passages_add_pass(); break;
    case CTRL('D'): level++; level_new(); break;
    case CTRL('E'): msg("food left: %d", food_left); break;
    case CTRL('F'): show_map(); break;
    case CTRL('I'): wizard_levels_and_gear(); break;
    case CTRL('T'): player_teleport(NULL); break;
    case CTRL('W'): identify(); break;
    case CTRL('X'): player_can_sense_monsters()
                    ? player_remove_sense_monsters()
                    : player_add_sense_monsters(true); break;
    case '*' : pr_list(); break;

    case CTRL('~'):
     {
       THING *item;

       if ((item = pack_get_item("charge", STICK)) != NULL)
         item->o_charges = 10000;
     }
     break;

    default:
      {
        char buf[MAXSTR];
        /* Message the player, but don't save it to ^P */
        strcpy(buf, huh);
        msg("illegal command '%s'", unctrl(ch));
        strcpy(huh, buf);
      }
      break;
  }
  return false;
}

void
command_signal_endit(int sig)
{
  (void)sig;
  endwin();
  puts("Okay, bye bye!\n");
  exit(0);
}

void
command_signal_quit(int sig)
{
  int oy, ox;
  (void) sig;

  getyx(curscr, oy, ox);
  msg("");
  msg("really quit? ");

  if (getch() == 'y')
  {
  /* Reset the signal in case we got here via an interrupt */
    signal(SIGINT, command_signal_leave);
    pack_evaluate();
    score(purse, 1, 0);
    exit(0);
  }
  else
  {
    status();
    msg("");
    move(oy, ox);
    refresh();
    command_stop(true);
  }
}

void
command_signal_leave(int sig)
{
  static char buf[BUFSIZ];
  (void)sig;

  setbuf(stdout, buf);	/* throw away pending output */

  if (!isendwin())
  {
    mvcur(0, COLS - 1, LINES - 1, 0);
    endwin();
  }

  putchar('\n');
  exit(0);
}

