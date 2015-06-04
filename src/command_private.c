#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>

#include "potions.h"
#include "scrolls.h"
#include "traps.h"
#include "options.h"
#include "io.h"
#include "pack.h"
#include "list.h"
#include "level.h"
#include "rings.h"
#include "misc.h"
#include "monster.h"
#include "player.h"
#include "weapons.h"
#include "wand.h"
#include "rip.h"
#include "daemons.h"
#include "move.h"
#include "rogue.h"

#include "command_private.h"

bool
command_use_stairs(char up_or_down)
{
  coord* player_pos = player_get_pos();

  assert (up_or_down == '>' || up_or_down == '<');

  if (player_is_levitating())
    msg("You can't. You're floating off the ground!");
  else if (level_get_ch(player_pos->y, player_pos->x) != STAIRS)
    msg("You're not standing on any stairs");

  else if (up_or_down == '>') /* DOWN */
  {
    level++;
    level_new();
  }

  else if (up_or_down == '<') /* UP */
  {
    bool has_amulet = pack_contains_amulet();

    assert(level >= 0 && "Level should not go lower than 0");

    if (level == 1)
    {
      if (has_amulet)
        total_winner();
      else
      {
        /* This either quits, or player did not want to leave */
        command_quit();
        return false;
      }
    }

    level--;
    level_new();

    if (has_amulet)
      msg("you feel a wrenching sensation in your gut");
  }

  return false;
}

bool
command_attack(bool fight_to_death)
{
  const coord* dir = get_dir();
  coord* player_pos = player_get_pos();

  if (dir == NULL)
    return false;

  coord delta = {
    .y = player_pos->y + dir->y,
    .x = player_pos->x + dir->x
  };

  kamikaze = fight_to_death;

  THING* mp = level_get_monster(delta.y, delta.x);
  if (mp == NULL || (!see_monst(mp) && !player_can_sense_monsters()))
  {
    msg("no monster there");
    return false;
  }
  else if (diag_ok(player_pos, &delta))
  {
    to_death = true;
    mp->t_flags |= ISTARGET;
    runch = dir_ch;
    return command_do(dir_ch);
  }
  else
    return true;
}

bool
command_name_item(void)
{
  THING* obj = pack_get_item("rename", RENAMEABLE);

  if (obj == NULL)
    return false;

  bool already_known;
  char** guess;
  switch (obj->o_type)
  {
    case FOOD: msg("Don't play with your food!"); return false;

    case RING:
      already_known = ring_is_known((enum ring_t)obj->o_which);
      guess =        &ring_info[obj->o_which].oi_guess;
      break;

    case POTION:
      already_known = pot_info[obj->o_which].oi_know;
      guess =        &pot_info[obj->o_which].oi_guess;
      break;

    case SCROLL:
      already_known = scroll_is_known((enum scroll_t)obj->o_which);
      guess =         NULL;
      break;

    case STICK:
      already_known = wand_is_known((enum wand_t)obj->o_which);
      guess =         NULL;
      break;

    default:
      already_known = false;
      guess = &obj->o_label;
      break;
  }

  if (already_known)
  {
    msg("that has already been identified");
    return false;
  }

  msg("what do you want to call it? ");

  char tmpbuf[MAXSTR] = { '\0' };
  if (readstr(tmpbuf) == 0)
  {
    if (obj->o_type == STICK)
      wand_set_name((enum wand_t)obj->o_which, tmpbuf);
    else if (obj->o_type == SCROLL)
      scroll_set_name((enum scroll_t)obj->o_which, tmpbuf);
    else if (guess != NULL)
    {
      if (*guess != NULL) {
        free(*guess);
        *guess = NULL;
      }
      if (strlen(tmpbuf) > 0)
      {
        *guess = malloc(strlen(tmpbuf) + 1);
        strcpy(*guess, tmpbuf);
      }
    }
  }

  clearmsg();
  return false;
}

bool
command_identify_character(void)
{
  msg("what do you want identified? ");
  int ch = readchar(true);
  mpos = 0;

  if (ch == KEY_ESCAPE)
  {
    clearmsg();
    return false;
  }

  if (isalpha(ch))
  {
    ch = toupper(ch);
    msg("'%s': %s", unctrl((chtype)ch), monsters[ch - 'A'].m_name);
    return false;
  }

  struct character_list
  {
    char ch;
    char const* description;
  } const ident_list[] = {
    {VWALL,   "wall of a room"}, {HWALL,   "wall of a room"},
    {GOLD,    "gold"},           {STAIRS,  "a staircase"},
    {DOOR,    "door"},           {FLOOR,   "room floor"},
    {PLAYER,  "you"},            {PASSAGE, "passage"},
    {TRAP,    "trap"},           {POTION,  "potion"},
    {SCROLL,  "scroll"},         {FOOD,    "food"},
    {WEAPON,  "weapon"},         {SHADOW,  "solid rock"},
    {ARMOR,   "armor"},          {AMULET,  "the Amulet of Yendor"},
    {RING,    "ring"},           {STICK,   "wand or staff"},
    {'\0', ""}
  };
  for (struct character_list const* ptr = ident_list; ptr->ch != '\0'; ++ptr)
    if (ptr->ch == ch)
    {
      msg("'%s': %s", unctrl((chtype)ch), ptr->description);
      return false;
    }

  msg("'%s': %s", unctrl((chtype)ch), "unknown character");
  return false;
}

bool
command_identify_trap(void)
{
  const coord* dir = get_dir();
  if (dir == NULL)
    return false;

  coord delta = {
    .y = player_y() + dir->y,
    .x = player_x() + dir->x
  };

  int flags = level_get_flags(delta.y, delta.x);

  if (level_get_ch(delta.y, delta.x) != TRAP)
    msg("no trap there");
  else if (player_has_confusing_attack())
    msg(trap_names[rnd(NTRAPS)]);
  else
  {
    msg(trap_names[flags & F_TMASK]);
    flags |= F_SEEN;
    level_set_flags(delta.y, delta.x, (char)flags);
  }
  return false;
}

bool
command_quit(void)
{
  command_signal_quit(0);
  return false;
}

bool
command_pick_up(void)
{
  if (player_is_levitating())
    msg("You can't. You're floating off the ground!");

  coord const* player_pos = player_get_pos();

  for (THING* obj = lvl_obj; obj != NULL; obj = obj->l_next)
    if (same_coords(&obj->o_pos, player_pos))
    {
      pack_pick_up(obj, true);
      return true;
    }

  msg("nothing to pick up");
  return false;
}

bool
command_help(void)
{
  struct list
  {
    char sym;
    char *description;
    bool print;
  } const helpstr[] = {
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
    {'f',	"	fight till death or near death",	true},
    {'t',	"	throw something",			true},
    {'z',	"	zap a wand in a direction",		true},
    {'^',	"	identify trap type",			true},
    {'s',	"	search for trap/secret door",		true},
    {'>',	"	go down a staircase",			true},
    {'<',	"	go up a staircase",			true},
    {'.',	"	rest for a turn",			true},
    {'Z',	"	rest until healed",			true},
    {',',	"	pick something up",			true},
    {'i',	"	inventory",				true},
    {'I',	"	equipment",				true},
    {'q',	"	quaff potion",				true},
    {'r',	"	read scroll",				true},
    {'e',	"	eat food",				true},
    {'w',	"	wield a weapon",			true},
    {'x',	"	wield last used weapon",		true},
    {'W',	"	wear armor",				true},
    {'T',	"	take armor off",			true},
    {'P',	"	put on ring",				true},
    {'R',	"	remove ring",				true},
    {'d',	"	drop object",				true},
    {'c',	"	call object",				true},
    {'a',	"	repeat last command",			true},
    {'D',	"	recall what's been discovered",		true},
    {'o',	"	examine/set options",			true},
    {CTRL('R'),	"	redraw screen",				true},
    {CTRL('P'),	"	repeat last message",			true},
    {KEY_ESCAPE,"	cancel command",			true},
    {'S',	"	save game",				true},
    {'Q',	"	quit",					true},
    {CTRL('Z'),	"	shell escape",				true},
    {'F',	"	fight till either of you dies",		true},
  };
  int const helpstrsize = sizeof(helpstr) / sizeof(*helpstr);

  msg("character you want help for (* for all): ");
  char helpch = readchar(true);
  mpos = 0;

  /* If its not a *, print the right help string
   * or an error if he typed a funny character. */
  if (helpch != '*')
  {
    move(0, 0);
    for (int i = 0; i < helpstrsize; ++i)
      if (helpstr[i].sym == helpch)
      {
        msg("%s)%s", unctrl(helpstr[i].sym), helpstr[i].description);
        return false;
      }
    msg("unknown character '%s'", unctrl(helpch));
    return false;
  }

  /* Here we print help for everything.
   * Then wait before we return to command mode */

  int numprint = 0;
  for (int i = 0; i < helpstrsize; ++i)
    if (helpstr[i].print)
      ++numprint;

  numprint /= 2;
  if (numprint > LINES - 1)
    numprint = LINES - 1;

  wclear(hw);
  int print_i = 0;
  for (int i = 0; i < helpstrsize; ++i)
  {
    if (!helpstr[i].print)
      continue;

    wmove(hw, print_i % numprint, print_i >= numprint ? COLS / 2 : 0);
    if (helpstr[i].sym)
      waddstr(hw, unctrl(helpstr[i].sym));
    waddstr(hw, helpstr[i].description);

    if (++print_i >= numprint * 2)
      break;
  }

  wmove(hw, LINES - 1, 0);
  waddstr(hw, "--Press space to continue--");
  wrefresh(hw);
  wait_for(KEY_SPACE);
  clearok(stdscr, true);

  clearmsg();
  touchwin(stdscr);
  wrefresh(stdscr);
  return false;
}

bool
command_again(void)
{
  if (last_comm != '\0')
  {
    again = true;
    return command_do(last_comm);
  }
  return false;
}


/* Let them escape for a while */
void
command_shell(void)
{
  /* Set the terminal back to original mode */
  move(LINES-1, 0);
  refresh();
  endwin();
  putchar('\n');
  fflush(stdout);

  /* Return to shell */
  raise(SIGSTOP);

  /* Set the terminal to gaming mode */
  fflush(stdout);
  noecho();
  raw();
  clearok(stdscr, true);
}

bool
command_show_inventory(void)
{
  if (pack_is_empty())
  {
    msg("You inventory is empty");
    return false;
  }

  pack_print_inventory(0);
  msg("--Press any key to continue--");
  readchar(false);
  pack_clear_inventory();
  clearmsg();
  return false;
}

bool
command_take_off(enum equipment_pos pos)
{
  if (pack_equipped_item(pos) == NULL)
    return false;

  pack_unequip(pos, false);
  return pack_equipped_item(pos) != NULL;
}

bool command_throw(void)
{
  const coord* dir = get_dir();
  return dir == NULL ? false : missile(dir->y, dir->x);
}

bool
command_wield(void)
{
  THING* obj = pack_get_item("wield", WEAPON);

  if (obj == NULL)
    return false;

  if (obj->o_type == ARMOR)
  {
    msg("you can't wield armor");
    return command_wield();
  }

  return weapon_wield(obj);
}

bool command_rest(void)
{
  for (THING* mon = mlist; mon != NULL; mon = mon->l_next)
    if (see_monst(mon))
    {
      msg("cannot rest with monsters nearby");
      return false;
    }

  if (!player_is_hurt())
  {
    msg("you don't feel the least bit tired");
    return false;
  }

  msg("you rest for a while");
  again = true;
  while (again && player_is_hurt())
  {
    daemon_run_before();
    daemon_run_after();
  }
  return true;
}

bool
command_eat(void)
{
  THING* obj = pack_get_item("eat", FOOD);
  if (obj == NULL)
    return false;

  if (obj->o_type != FOOD)
  {
    msg("that's inedible!");
    return false;
  }

  food_left = (food_left > 0 ? food_left : 0) + HUNGERTIME - 200 + rnd(400);
  if (food_left > STOMACHSIZE)
    food_left = STOMACHSIZE;

  hungry_state = 0;

  if (obj->o_which == 1)
    msg("my, that was a yummy fruit");

  else if (rnd(100) > 70)
  {
    player_earn_exp(1);
    msg("this food tastes awful");
    player_check_for_level_up();
  }

  else
    msg("that tasted good");

  pack_remove(obj, false, false);
  return true;
}

bool
command_run(char ch, bool cautiously)
{
  if (cautiously)
  {
    door_stop = true;
    firstmove = true;
  }

  running = true;
  runch = (char)tolower(ch);
  return move_do(runch);
}

bool command_drop(void)
{
  coord* player_pos = player_get_pos();
  char ch = level_get_ch(player_pos->y, player_pos->x);

  if (ch != FLOOR && ch != PASSAGE)
  {
    msg("there is something there already");
    return false;
  }

  THING* obj = pack_get_item("drop", 0);
  if (obj == NULL)
    return false;

  bool drop_all = false;
  if (obj->o_count > 1)
  {
    msg("Drop all? (y/N) ");
    drop_all = readchar(true) == 'y';
  }

  obj = pack_remove(obj, true, drop_all);

  /* Link it into the level object list */
  list_attach(&lvl_obj, obj);

  level_set_ch(player_pos->y, player_pos->x, (char)obj->o_type);
  int flags = level_get_flags(player_pos->y, player_pos->x);
  flags |= F_DROPPED;
  level_set_flags(player_pos->y, player_pos->x, (char)flags);

  obj->o_pos = *player_pos;

  char buf[MAXSTR];
  msg("dropped %s", inv_name(buf, obj, true));
  return true;
}

