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
#include <ctype.h>

#include "potions.h"
#include "status_effects.h"
#include "scrolls.h"
#include "options.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "daemons.h"
#include "list.h"
#include "move.h"
#include "new_level.h"
#include "passages.h"
#include "rings.h"
#include "save.h"
#include "rogue.h"

#include "command_private.h"

void
command_stop(bool stop_fighting)
{
  player.t_flags &= ~ISRUN;
  count = 0;
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
  int player_moves = on(player, ISHASTE) ? 2 : 1;

  /* Let the daemons start up */
  daemon_run_all(BEFORE);
  daemon_run_fuses(BEFORE);
  for (; player_moves > 0; --player_moves)
  {
    /* these are illegal things for the player to be, so if any are
     * set, someone's been poking in memeory */
    if (on(player, ISSLOW|ISGREED|ISINVIS|ISREGEN|ISTARGET))
      return 1;

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
    move(hero.y, hero.x);
    if (!((running || count) && jump))
      refresh();
    take = 0;
    after = true;


    if (no_command)
    {
      ch = '.';
      if (--no_command == 0)
      {
        player.t_flags |= ISRUN;
        msg("you can move again");
      }
    }

    if (!no_command)
    {
      if (running || to_death)
        ch = runch;
      else if (!count)
      {
        ch = readchar();
        move_on = false;
        if (mpos != 0)
          msg("");
      }

      /* check for prefixes */
      if (isdigit(ch))
      {
        count = 0;
        while (isdigit(ch))
        {
          count = count * 10 + (ch - '0');
          if (count > 255)
            count = 255;
          ch = readchar();
        }

        /* turn off count for commands which don't make sense to repeat */
        switch (ch)
        {
          case CTRL('B'): case CTRL('H'): case CTRL('J'):
          case CTRL('K'): case CTRL('L'): case CTRL('N'):
          case CTRL('U'): case CTRL('Y'):
          case '.': case 'a': case 'b': case 'h': case 'j':
          case 'k': case 'l': case 'm': case 'n': case 'q':
          case 'r': case 's': case 't': case 'u': case 'y':
          case 'z': case 'B': case 'C': case 'H': case 'I':
          case 'J': case 'K': case 'L': case 'N': case 'U':
          case 'Y':
          case CTRL('D'): case CTRL('A'):
            break;
          default:
            count = 0;
        }
      }

      /* execute a command */
      if (count && !running)
      {
        msg("Count remaining: %d", --count);
        mpos = 0;
      }

      if (ch != 'a' && ch != KEY_ESCAPE && !(running || count || to_death))
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
      player_moves++;
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
        teleport(&player, NULL);
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
    case 'r': read_scroll(); return true;
    case 's': return command_search();
    case 't': return get_dir() ? missile(delta.y, delta.x) : false;
    case 'w': return wield();
    case 'z': return get_dir() ? do_zap() : false;

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
    case CTRL('Z'): shell(); return false;

    default:
      if (wizard)
        return command_wizard_do(ch);
      else
      {
        char buf[MAXSTR];
        /* Message the player, but don't save it to ^P */
        count = 0;
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
    case '|': msg("@ %d,%d", hero.y, hero.x);
    when 'C': create_obj();
    when '$': msg("inpack = %d", pack_count_items());
    /* when '\\': get_dir(); teleport(moat(delta.y + hero.y, hero.x + delta.x), NULL); * This is used for testing new features */
    when CTRL('W'): whatis(0);
    when CTRL('D'): level++; new_level();
    when CTRL('A'): level--; new_level();
    when CTRL('F'): show_map();
    when CTRL('T'): teleport(&player, NULL);
    when CTRL('E'): msg("food left: %d", food_left);
    when CTRL('C'): passages_add_pass();
    when CTRL('X'): turn_see(on(player, SEEMONST));
    when '*' : pr_list();

    when CTRL('~'):
   {
     THING *item;

     if ((item = pack_get_item("charge", STICK)) != NULL)
       item->o_charges = 10000;
   }

    when CTRL('I'):
    {
      int i;
      THING *obj;

      for (i = 0; i < 9; i++)
        raise_level();

      /* Give him a sword (+1,+1) */
      if (pack_unequip(EQUIPMENT_RHAND))
      {
        obj = new_item();
        init_weapon(obj, TWOSWORD);
        obj->o_hplus = 1;
        obj->o_dplus = 1;
        pack_add(obj, true);
        pack_equip_item(obj);
      }
      else
        msg("failed to add weapon");

      /* And his suit of armor */
      if (pack_unequip(EQUIPMENT_ARMOR))
      {
        obj = new_item();
        obj->o_type = ARMOR;
        obj->o_which = PLATE_MAIL;
        obj->o_arm = -5;
        obj->o_flags |= ISKNOW;
        obj->o_count = 1;
        obj->o_group = 0;
        pack_equip_item(obj);
      }
      else
        msg("failed to add armor");
    }

    otherwise:
      {
        char buf[MAXSTR];
        /* Message the player, but don't save it to ^P */
        count = 0;
        strcpy(buf, huh);
        msg("illegal command '%s'", unctrl(ch));
        strcpy(huh, buf);
      }
  }
  return false;
}

