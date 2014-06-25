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

/* Local functions */
static bool do_command(char ch);
static bool do_wizard_command(char ch);
static bool pick_up_item_from_ground();
static void bad_command(int ch);      /* Tell player she's pressed wrong keys */
static void search();                /* Find traps, hidden doors and passages */
static void print_help();                 /* Give command help                */
static void identify_a_character();       /* Identify monster and items       */
static void go_down_a_level();            /* Go down a dungeon level          */
static void go_up_a_level();              /* Go up a dungeon_level            */
static bool levit_check();                /* Check to see if she's levitating */
static void give_item_nickname();         /* Call an item something           */
static bool print_currently_wearing(char thing); /* Print weapon / armor info */

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
      refresh();			/* Draw screen */
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

        /* turn off count for commands which don't make sense
         * to repeat */
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
      if (ch != 'a' && ch != ESCAPE && !(running || count || to_death))
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

    /*TODO: change do_move to take a char instead of x,y */
    case 'h': case 'j': case 'k': case 'l':
    case 'y': case 'u': case 'b': case 'n':
      return do_move(ch);

    case 'H': case 'J': case 'K': case 'L':
    case 'Y': case 'U': case 'B': case 'N':
      return do_run(tolower(ch));

    case CTRL('H'): case CTRL('J'): case CTRL('K'): case CTRL('L'):
    case CTRL('Y'): case CTRL('U'): case CTRL('B'): case CTRL('N'):
      if (!is_blind(player))
      {
        door_stop = true;
        firstmove = true;
      }
      return do_command(ch + ('A' - CTRL('A')));

    case 'F':
      kamikaze = true;
    /* FALLTHROUGH */
    case 'f':
    {
      THING *mp;
      if (!get_dir())
        return false;
      delta.y += hero.y;
      delta.x += hero.x;
      if ((mp = moat(delta.y, delta.x)) == NULL ||
          (!see_monst(mp) && !on(player, SEEMONST)))
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
        runch = ch = dir_ch;
        return do_command(ch);
      }
    }
    return true;

    case 'a':
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

    case 'Q':
      /* TODO: remove q_comm */
      q_comm = true;
      quit(0);
      q_comm = false;
      return false;

    case CTRL('R'):
      clearok(curscr,true);
      wrefresh(curscr);
      return false;

    case '^':
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
        else if (is_hallucinating(player))
          msg(tr_name[rnd(NTRAPS)]);
        else
        {
          msg(tr_name[*fp & F_TMASK]);
          *fp |= F_SEEN;
        }
      }
      return false;

    case '+':
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

    case ESCAPE:
      door_stop = false;
      count = 0;
      again = false;
      return false;

    /* Funny symbols */
    case KEY_SPACE: return false;
    case '.': return true;
    case ',': return pick_up_item_from_ground();
    case '/': identify_a_character(); return false;
    case '>': go_down_a_level(); return false;
    case '<': go_up_a_level(); return false;
    case '?': print_help(); return false;
    case '!': msg("Shell has been removed, use ^Z instead"); return false;
    case '@': status(true); return false;
    case ')': return print_currently_wearing(WEAPON);
    case ']': return print_currently_wearing(ARMOR);
    case '=': return print_currently_wearing(RING);

    /* Lower case */
    case 'c': give_item_nickname(); return false;
    case 'd': drop(); return true;
    case 'e': eat(); return true;
    case 'i': inventory(pack, 0); return false;
    case 'm': move_on = true; return get_dir() ? do_command(dir_ch) : false;
    case 'o': option(); return false;
    case 'q': return quaff();
    case 'r': read_scroll(); return true;
    case 's': search(); return true;
    case 't': return get_dir() ? missile(delta.y, delta.x) : false;
    case 'w': return wield();
    case 'z': return get_dir() ? do_zap() : false;

    /* Upper case */
    case 'D': discovered(); return false;
    case 'I': picky_inven(); return false;
    case 'P': return ring_on();
    case 'R': return ring_off();
    case 'S': after = false; save_game(); return false;
    case 'T': return take_off();
    case 'W': return wear();

    /* Ctrl case */
    case CTRL('P'): msg(huh); return false;
    case CTRL('Z'): shell(); return false;

    default:
      return do_wizard_command(ch);
  }
}

static bool
do_wizard_command(char ch)
{
  after = false;
  if (!wizard)
    return false;

  switch (ch)
  {
    case '|': msg("@ %d,%d", hero.y, hero.x);
    when 'C': create_obj();
    when '$': msg("inpack = %d", inpack);
    when CTRL('G'): inventory(lvl_obj, 0);
    when CTRL('W'): whatis(false, 0);
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

static bool
pick_up_item_from_ground()
{
  THING *obj = NULL;
  for (obj = lvl_obj; obj != NULL; obj = next(obj))
    if (obj->o_pos.y == hero.y && obj->o_pos.x == hero.x)
    {
      if (!levit_check())
        pick_up((char)obj->o_type);
      return true;
    }

  if (!terse)
    addmsg("there is ");
  addmsg("nothing here");
  if (!terse)
    addmsg(" to pick up");
  endmsg();
  return false;
}

static void
bad_command(int ch)
{
    save_msg = false;
    count = 0;
    msg("illegal command '%s'", unctrl(ch));
    save_msg = true;
}

static void
search()
{
  int y, x;
  int probinc = (is_hallucinating(player) ? 3:0) + is_blind(player) ? 2:0;
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

        if (is_hallucinating(player))
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
print_help()
{
  struct h_list helpstr[] = {
    {'?',	"	prints help",				true},
    {'/',	"	identify object",			true},
    {'h',	"	left",					true},
    {'j',	"	down",					true},
    {'k',	"	up",					true},
    {'l',	"	right",					true},
    {'y',	"	up & left",				true},
    {'u',	"	up & right",				true},
    {'b',	"	down & left",				true},
    {'n',	"	down & right",				true},
    {'H',	"	run left",				false},
    {'J',	"	run down",				false},
    {'K',	"	run up",				false},
    {'L',	"	run right",				false},
    {'Y',	"	run up & left",				false},
    {'U',	"	run up & right",			false},
    {'B',	"	run down & left",			false},
    {'N',	"	run down & right",			false},
    {CTRL('H'),	"	run left until adjacent",		false},
    {CTRL('J'),	"	run down until adjacent",		false},
    {CTRL('K'),	"	run up until adjacent",			false},
    {CTRL('L'),	"	run right until adjacent",		false},
    {CTRL('Y'),	"	run up & left until adjacent",		false},
    {CTRL('U'),	"	run up & right until adjacent",		false},
    {CTRL('B'),	"	run down & left until adjacent",	false},
    {CTRL('N'),	"	run down & right until adjacent",	false},
    {'\0',	"	<SHIFT><dir>: run that way",		true},
    {'\0',	"	<CTRL><dir>: run till adjacent",	true},
    {'f',	"<dir>	fight till death or near death",	true},
    {'t',	"<dir>	throw something",			true},
    {'m',	"<dir>	move onto without picking up",		true},
    {'z',	"<dir>	zap a wand in a direction",		true},
    {'^',	"<dir>	identify trap type",			true},
    {'s',	"	search for trap/secret door",		true},
    {'>',	"	go down a staircase",			true},
    {'<',	"	go up a staircase",			true},
    {'.',	"	rest for a turn",			true},
    {',',	"	pick something up",			true},
    {'i',	"	inventory",				true},
    {'I',	"	inventory single item",			true},
    {'q',	"	quaff potion",				true},
    {'r',	"	read scroll",				true},
    {'e',	"	eat food",				true},
    {'w',	"	wield a weapon",			true},
    {'W',	"	wear armor",				true},
    {'T',	"	take armor off",			true},
    {'P',	"	put on ring",				true},
    {'R',	"	remove ring",				true},
    {'d',	"	drop object",				true},
    {'c',	"	call object",				true},
    {'a',	"	repeat last command",			true},
    {')',	"	print current weapon",			true},
    {']',	"	print current armor",			true},
    {'=',	"	print current rings",			true},
    {'@',	"	print current stats",			true},
    {'D',	"	recall what's been discovered",		true},
    {'o',	"	examine/set options",			true},
    {CTRL('R'),	"	redraw screen",				true},
    {CTRL('P'),	"	repeat last message",			true},
    {ESCAPE,	"	cancel command",			true},
    {'S',	"	save game",				true},
    {'Q',	"	quit",					true},
    {'!',	"	shell escape",				true},
    {'F',	"<dir>	fight till either of you dies",		true},
    { 0 ,		NULL,					false}
  };
  struct h_list *strp;
  char helpch;
  int numprint;
  int cnt;

  msg("character you want help for (* for all): ");
  helpch = readchar();
  mpos = 0;

  /* If its not a *, print the right help string
   * or an error if he typed a funny character. */
  if (helpch != '*')
  {
    move(0, 0);
    for (strp = helpstr; strp->h_desc != NULL; strp++)
      if (strp->h_ch == helpch)
      {
        lower_msg = true;
        msg("%s%s", unctrl(strp->h_ch), strp->h_desc);
        lower_msg = false;
        return;
      }
    msg("unknown character '%s'", unctrl(helpch));
    return;
  }

  /* Here we print help for everything.
   * Then wait before we return to command mode */

  numprint = 0;
  for (strp = helpstr; strp->h_desc != NULL; strp++)
    if (strp->h_print)
      numprint++;
  if (numprint & 01)		/* round odd numbers up */
    numprint++;
  numprint /= 2;
  if (numprint > LINES - 1)
    numprint = LINES - 1;

  wclear(hw);
  cnt = 0;
  for (strp = helpstr; strp->h_desc != NULL; strp++)
    if (strp->h_print)
    {
      wmove(hw, cnt % numprint, cnt >= numprint ? COLS / 2 : 0);
      if (strp->h_ch)
        waddstr(hw, unctrl(strp->h_ch));
      waddstr(hw, strp->h_desc);
      if (++cnt >= numprint * 2)
        break;
    }
  wmove(hw, LINES - 1, 0);
  waddstr(hw, "--Press space to continue--");
  wrefresh(hw);
  wait_for(KEY_SPACE);
  clearok(stdscr, true);

  msg("");
  touchwin(stdscr);
  wrefresh(stdscr);
}

static void
identify_a_character()
{
  int ch;
  char *str;
  struct h_list ident_list[] = {
    {VWALL,	"wall of a room",	false},
    {HWALL,	"wall of a room",	false},
    {GOLD,	"gold",			false},
    {STAIRS,	"a staircase",		false},
    {DOOR,	"door",			false},
    {FLOOR,	"room floor",		false},
    {PLAYER,	"you",			false},
    {PASSAGE,	"passage",		false},
    {TRAP,	"trap",			false},
    {POTION,	"potion",		false},
    {SCROLL,	"scroll",		false},
    {FOOD,	"food",			false},
    {WEAPON,	"weapon",		false},
    {SHADOW,	"solid rock",		false},
    {ARMOR,	"armor",		false},
    {AMULET,	"the Amulet of Yendor",	false},
    {RING,	"ring",			false},
    {STICK,	"wand or staff",	false},
    {'\0'}
  };

  msg("what do you want identified? ");
  ch = readchar();
  mpos = 0;
  if (ch == ESCAPE)
  {
    msg("");
    return;
  }
  if (islower(ch))
    ch = toupper(ch);
  if (isupper(ch))
    str = monsters[ch-'A'].m_name;
  else
  {
    struct h_list *hp;
    str = "unknown character";
    for (hp = ident_list; hp->h_ch != '\0'; hp++)
      if (hp->h_ch == ch)
      {
        str = hp->h_desc;
        break;
      }
  }
  msg("'%s': %s", unctrl(ch), str);
}

static void
go_down_a_level()
{
  if (levit_check())
    return;
  if (chat(hero.y, hero.x) != STAIRS)
    msg("I see no way down");
  else
  {
    level++;
    seenstairs = false;
    new_level();
  }
}

static void
go_up_a_level()
{
  if (levit_check())
    return;
  if (chat(hero.y, hero.x) == STAIRS)
    if (amulet)
    {
      level--;
      if (level == 0)
        total_winner();
      new_level();
      msg("you feel a wrenching sensation in your gut");
    }
    else
      msg("your way is magically blocked");
  else
    msg("I see no way up");
}

static bool
levit_check()
{
  if (is_levitating(player))
  {
    msg("You can't. You're floating off the ground!");
    return true;
  }
  return false;
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
