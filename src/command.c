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
#include "command_sub.h"

/* Local functions */
static bool do_command(char ch);
static bool do_wizard_command(char ch);

static void bad_command(int ch);      /* Tell player she's pressed wrong keys */
static void search();                /* Find traps, hidden doors and passages */
static void give_item_nickname();         /* Call an item something           */
static bool print_currently_wearing(char thing); /* Print weapon / armor info */
static bool fight_monster(bool fight_to_death); /* Attack and fight something */
static bool toggle_wizard();              /* Toggle wizard-mode on or off     */
static bool identify_trap();              /* Give the name of a trap          */
static bool maybe_quit();                 /* Ask player if she wants to quit  */
static bool repeat_last_command();
static bool festina_lente(char ch);       /* Run cautiously                   */
static bool show_players_inventory();

/* command:
 * Process the user commands */
void
command()
{
  char ch;
  int player_moves = on(player, ISHASTE) ? 2 : 1;
  static char countch;

  /* Let the daemons start up */
  do_daemons(BEFORE);
  do_fuses(BEFORE);
  for (; player_moves > 0; --player_moves)
  {
    /* these are illegal things for the player to be, so if any are
     * set, someone's been poking in memeory */
    if (on(player, ISSLOW|ISGREED|ISINVIS|ISREGEN|ISTARGET))
      exit(1);

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
    status(false);
    lastscore = purse;
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
      else if (count)
        ch = countch;
      else
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
        countch = ch;

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
        count--;
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
    teleport();
  if (ISRING(RIGHT, R_SEARCH))
    search();
  else if (ISRING(RIGHT, R_TELEPORT) && rnd(50) == 0)
    teleport();
}

static bool
do_command(char ch)
{
  switch (ch)
  {
    /* Funny symbols */
    case KEY_SPACE: return false;
    case KEY_ESCAPE: door_stop = again = false; count = 0; return false;
    case '.': return true;
    case ',': return pick_up_item_from_ground();
    case '/': return identify_a_character();
    case '>': return change_dungeon_level(DOWN);
    case '<': return change_dungeon_level(UP);
    case '?': return print_help();
    case '!': msg("Shell has been removed, use ^Z instead"); return false;
    case '@': status(true); return false;
    case '^': return identify_trap();
    case ')': return print_currently_wearing(WEAPON);
    case ']': return print_currently_wearing(ARMOR);
    case '+': return toggle_wizard();
    case '=': return print_currently_wearing(RING);

    /* Lower case */
    case 'h': case 'j': case 'k': case 'l':
    case 'y': case 'u': case 'b': case 'n':
      return do_move(ch);
    case 'a': return repeat_last_command();
    case 'c': give_item_nickname(); return false;
    case 'd': return drop();
    case 'e': eat(); return true;
    case 'f': return fight_monster(false);
    case 'i': return show_players_inventory();
    case 'm': move_on = true; return get_dir() ? do_command(dir_ch) : false;
    case 'o': option(); return false;
    case 'q': return quaff();
    case 'r': read_scroll(); return true;
    case 's': search(); return true;
    case 't': return get_dir() ? missile(delta.y, delta.x) : false;
    case 'w': return wield();
    case 'z': return get_dir() ? do_zap() : false;

    /* Upper case */
    case 'H': case 'J': case 'K': case 'L':
    case 'Y': case 'U': case 'B': case 'N':
      return do_run(tolower(ch));
    case 'D': discovered(); return false;
    case 'F': return fight_monster(true);
    case 'I': picky_inven(); return false;
    case 'P': return ring_on();
    case 'R': return ring_off();
    case 'S': after = false; save_game(); return false;
    case 'T': return take_off();
    case 'W': return wear();
    case 'Q': return maybe_quit();

    /* Ctrl case */
    case CTRL('H'): case CTRL('J'): case CTRL('K'): case CTRL('L'):
    case CTRL('Y'): case CTRL('U'): case CTRL('B'): case CTRL('N'):
      return festina_lente(ch);
    case CTRL('P'): msg(huh); return false;
    case CTRL('R'): clearok(curscr, true); wrefresh(curscr); return false;
    case CTRL('Z'): shell(); return false;

    default:
      if (wizard)
        return do_wizard_command(ch);
      else
      {
        bad_command(ch);
        return false;
      }
  }
}

static bool
do_wizard_command(char ch)
{
  after = false;

  switch (ch)
  {
    case '|': msg("@ %d,%d", hero.y, hero.x);
    when 'C': create_obj();
    when '$': msg("inpack = %d", items_in_pack());
    /* when '\\': This is used for testing new features */
    when CTRL('W'): whatis(0);
    when CTRL('D'): level++; new_level();
    when CTRL('A'): level--; new_level();
    when CTRL('F'): show_map();
    when CTRL('T'): teleport();
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
      bad_command(ch);
  }
  return false;
}

static void
bad_command(int ch)
{
    count = 0;
    unsaved_msg("illegal command '%s'", unctrl(ch));
}

static void
search()
{
  int y, x;
  int probinc = (is_hallucinating(&player) ? 3:0) + is_blind(&player) ? 2:0;
  bool found = false;

  for (y = hero.y - 1; y <= hero.y + 1; y++)
    for (x = hero.x - 1; x <= hero.x + 1; x++)
    {
      char *fp = &flat(y, x);
      char chatyx = chat(y, x);

      /* Real wall/floor/shadow */
      if (*fp & F_REAL)
        continue;

      /* Wall */
      if ((chatyx == VWALL || chatyx == HWALL) &&
          rnd(5 + probinc) == 0)
      {
        chat(y, x) = DOOR;
        msg("a secret door");
        found = true;
        *fp |= F_REAL;
      }

      /* Floor */
      if (chatyx == FLOOR && rnd(2 + probinc) == 0)
      {
        chat(y, x) = TRAP;

        if (!terse)
          addmsg("you found ");

        if (is_hallucinating(&player))
          msg(tr_name[rnd(NTRAPS)]);
        else {
          msg(tr_name[*fp & F_TMASK]);
          *fp |= F_SEEN;
        }

        found = true;
        *fp |= F_REAL;
      }

      /* Shadow */
      if (chatyx == SHADOW && rnd(3 + probinc) == 0)
      {
        chat(y, x) = PASSAGE;
        found = true;
        *fp |= F_REAL;
      }
    }
  if (found)
  {
    look(false);
    count = false;
    running = false;
  }
}

static void
give_item_nickname()
{
  THING *obj = get_item("call", CALLABLE);
  char **guess;
  char *elsewise = NULL;
  bool already_known = false;
  char tmpbuf[MAXSTR] = { '\0' };

  /* Make certain that it is somethings that we want to wear */
  if (obj == NULL)
    return;

  switch (obj->o_type)
  {
    struct obj_info *op = NULL;

    case FOOD: msg("you can't call that anything"); return;

    case RING:
      op = &ring_info[obj->o_which];
      already_known = op->oi_know;
      guess = &op->oi_guess;
      elsewise = *guess ? *guess : r_stones[obj->o_which];

    when POTION:
      op = &pot_info[obj->o_which];
      already_known = op->oi_know;
      guess = &op->oi_guess;
      elsewise = *guess ? *guess : p_colors[obj->o_which];

    when SCROLL:
      op = &scr_info[obj->o_which];
      already_known = op->oi_know;
      guess = &op->oi_guess;
      elsewise = *guess ? *guess : s_names[obj->o_which];

    when STICK:
      op = &ws_info[obj->o_which];
      already_known = op->oi_know;
      guess = &op->oi_guess;
      elsewise = *guess ? *guess : ws_made[obj->o_which];

    otherwise:
      guess = &obj->o_label;
      elsewise = obj->o_label;
  }

  if (already_known)
  {
    msg("that has already been identified");
    return;
  }

  if (elsewise != NULL && elsewise == *guess)
    msg("Was called \"%s\"", elsewise);

  if (!terse)
    addmsg("What do you want to ");
  msg("call it? ");

  if (get_str(tmpbuf, stdscr) == NORMAL)
  {
    if (*guess != NULL) {
      free(*guess);
      *guess = NULL;
    }
    if (strlen(tmpbuf) > 0)
    {
      *guess = malloc((unsigned int) strlen(tmpbuf) + 1);
      strcpy(*guess, tmpbuf);
    }
  }
}

static bool
print_currently_wearing(char thing)
{
  bool item_found = false;

  inv_describe = false;
  if (!terse)
    addmsg("You are %s ", thing == WEAPON ? "wielding" : "wearing");

  if (thing == RING)
  {
    unsigned i;
    for (i = 0; i < CONCURRENT_RINGS; ++i)
      if (cur_ring[i]) {
        addmsg("%c) %s ", cur_ring[i]->o_packch, inv_name(cur_ring[i], true));
        item_found = true;
      }
    if (item_found)
      endmsg();

  }
  else
  {
    THING *current_thing = thing == WEAPON ? cur_weapon : cur_armor;
    if (current_thing) {
      msg("%c) %s", current_thing->o_packch, inv_name(current_thing, true));
      item_found = true;
    }
  }

  if (!item_found)
    msg("no %s", thing == WEAPON ? "weapon": thing == RING ? "rings":"armor");
  inv_describe = true;

  return false;
}

static bool
fight_monster(bool fight_to_death)
{
  THING *mp;

  kamikaze = fight_to_death;
  if (!get_dir())
    return false;
  delta.y += hero.y;
  delta.x += hero.x;

  mp = moat(delta.y, delta.x);
  if (mp == NULL || (!see_monst(mp) && !on(player, SEEMONST)))
  {
    if (!terse)
      addmsg("I see ");
    msg("no monster there");
    return false;
  }
  else if (diag_ok(&hero, &delta))
  {
    to_death = true;
    max_hit = 0;
    mp->t_flags |= ISTARGET;
    runch = dir_ch;
    return do_command(dir_ch);
  }
  else
    return true;
}

static bool
toggle_wizard()
{
  /* TODO: Add a query here, so you always can become a wiz */
  if (potential_wizard)
  {
    wizard = !wizard;
    turn_see(!wizard);
    if (wizard)
      msg("You are one with the force (seed: #%d)", seed);
    else
      msg("not wizard any more");
  }
  return false;
}

static bool
identify_trap()
{
  if (get_dir())
  {
    char *fp;
    delta.y += hero.y;
    delta.x += hero.x;
    fp = &flat(delta.y, delta.x);
    if (!terse)
      addmsg("You have found ");
    if (chat(delta.y, delta.x) != TRAP)
      msg("no trap there");
    else if (is_hallucinating(&player))
      msg(tr_name[rnd(NTRAPS)]);
    else
    {
      msg(tr_name[*fp & F_TMASK]);
      *fp |= F_SEEN;
    }
  }
  return false;
}

static bool
maybe_quit()
{
  quit(0);
  return false;
}

static bool
repeat_last_command()
{
  if (last_comm == '\0')
  {
    msg("you haven't typed a command yet");
    return false;
  }
  else
  {
    again = true;
    return do_command(last_comm);
  }
}

static bool
festina_lente(char ch)
{
  if (!is_blind(&player))
  {
    door_stop = true;
    firstmove = true;
  }
  return do_command(ch + ('A' - CTRL('A')));
}

static bool
show_players_inventory()
{
  print_inventory(0);
  msg("--Press any key to continue--");
  getch();
  clear_inventory();
  msg("");
  return false;
}
