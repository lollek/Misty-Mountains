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

#include "rogue.h"
#include "potions.h"
#include "status_effects.h"
#include "scrolls.h"
#include "options.h"
#include "io.h"
#include "armor.h"
#include "pack.h"

#include "command_private.h"

void
stop_counting(bool stop_fighting)
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
  do_daemons(BEFORE);
  do_fuses(BEFORE);
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
      after = do_command(ch);

      /* turn off flags if no longer needed */
      if (!running)
        door_stop = false;
    }

    /* If he ran into something to take, let him pick it up.  */
    if (take != 0)
      pick_up(take);
    if (!running)
      door_stop = false;
    if (!after)
      player_moves++;
  }

  do_daemons(AFTER);
  do_fuses(AFTER);

  if (ISRING(LEFT, R_SEARCH))
    search();
  else if (ISRING(LEFT, R_TELEPORT) && rnd(50) == 0)
    teleport(&player, NULL);
  if (ISRING(RIGHT, R_SEARCH))
    search();
  else if (ISRING(RIGHT, R_TELEPORT) && rnd(50) == 0)
    teleport(&player, NULL);

  return 0; /* Restart from top */
}

bool
do_command(char ch)
{
  switch (ch)
  {
    /* Funny symbols */
    case KEY_SPACE: return false;
    case KEY_ESCAPE: stop_counting(true); return false;
    case '.': return true;
    case ',': return pick_up_item_from_ground();
    case '/': return identify_a_character();
    case '>': return change_dungeon_level(ch);
    case '<': return change_dungeon_level(ch);
    case '?': return print_help();
    case '!': msg("Shell has been removed, use ^Z instead"); return false;
    case '^': return identify_trap();
    case ')': return print_currently_wearing(WEAPON);
    case ']': return print_currently_wearing(ARMOR);
    case '+': return toggle_wizard_mode();
    case '=': return print_currently_wearing(RING);

    /* Lower case */
    case 'h': case 'j': case 'k': case 'l':
    case 'y': case 'u': case 'b': case 'n':
      return do_move(ch);
    case 'a': return repeat_last_command();
    case 'c': return give_item_nickname();
    case 'd': return drop();
    case 'e': eat(); return true;
    case 'f': return fight_monster(false);
    case 'i': return show_players_inventory();
    case 'm': move_on = true; return get_dir() ? do_command(dir_ch) : false;
    case 'o': return option();
    case 'q': return quaff();
    case 'r': read_scroll(); return true;
    case 's': return search();
    case 't': return get_dir() ? missile(delta.y, delta.x) : false;
    case 'w': return wield();
    case 'z': return get_dir() ? do_zap() : false;

    /* Upper case */
    case 'H': case 'J': case 'K': case 'L':
    case 'Y': case 'U': case 'B': case 'N':
      return do_run(ch, false);
    case 'D': discovered(); return false;
    case 'F': return fight_monster(true);
    case 'P': return ring_on();
    case 'R': return ring_off();
    case 'S': return save_game();
    case 'T': return take_off();
    case 'W': return wear();
    case 'Q': return maybe_quit();

    /* Ctrl case */
    case CTRL('H'): case CTRL('J'): case CTRL('K'): case CTRL('L'):
    case CTRL('Y'): case CTRL('U'): case CTRL('B'): case CTRL('N'):
      return do_run(UNCTRL(ch), true);
    case CTRL('P'): msg(huh); return false;
    case CTRL('R'): clearok(curscr, true); wrefresh(curscr); return false;
    case CTRL('Z'): shell(); return false;

    default:
      if (wizard)
        return do_wizard_command(ch);
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
do_wizard_command(char ch)
{
  after = false;

  switch (ch)
  {
    case '|': msg("@ %d,%d", hero.y, hero.x);
    when 'C': create_obj();
    when '$': msg("inpack = %d", items_in_pack());
    /* when '\\': get_dir(); teleport(moat(delta.y + hero.y, hero.x + delta.x), NULL); * This is used for testing new features */
    when CTRL('W'): whatis(0);
    when CTRL('D'): level++; new_level();
    when CTRL('A'): level--; new_level();
    when CTRL('F'): show_map();
    when CTRL('T'): teleport(&player, NULL);
    when CTRL('E'): msg("food left: %d", food_left);
    when CTRL('C'): add_pass();
    when CTRL('X'): turn_see(on(player, SEEMONST));
    when '*' : pr_list();

    when CTRL('~'):
   {
     THING *item;

     if ((item = get_item("charge", STICK)) != NULL)
       item->o_charges = 10000;
   }

    when CTRL('I'):
    {
      int i;
      THING *obj;

      for (i = 0; i < 9; i++)
        raise_level();

      /* Give him a sword (+1,+1) */
      obj = new_item();
      init_weapon(obj, TWOSWORD);
      obj->o_hplus = 1;
      obj->o_dplus = 1;
      add_pack(obj, true);
      cur_weapon = obj;

      /* And his suit of armor */
      obj = new_item();
      obj->o_type = ARMOR;
      obj->o_which = PLATE_MAIL;
      obj->o_arm = -5;
      obj->o_flags |= ISKNOW;
      obj->o_count = 1;
      obj->o_group = 0;
      cur_armor = obj;
      add_pack(obj, true);
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

