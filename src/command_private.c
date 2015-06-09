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
#include "fight.h"
#include "food.h"
#include "rogue.h"

#include "command_private.h"

static bool command_attack_bow(coord const* delta)
{
  THING* ptr = pack_find_arrow();

  if (ptr == NULL)
  {
    msg("you've run out of arrows!");
    return true;
  }

  THING* arrow = pack_remove(ptr, true, false);
  do_motion(arrow, delta->y, delta->x);
  THING* monster_at_pos = level_get_monster(arrow->o_pos.y, arrow->o_pos.x);

  if (monster_at_pos == NULL || !fight_against_monster(&arrow->o_pos, arrow, true))
    fall(arrow, true);

  return true;
}

static bool command_attack_melee(bool fight_to_death, coord* delta)
{
  THING* mp = level_get_monster(delta->y, delta->x);
  if (mp != NULL && diag_ok(player_get_pos(), delta))
  {
    if (fight_to_death)
    {
      to_death = true;
      mp->t_flags |= ISTARGET;
    }
    runch = dir_ch;
    return command_do(dir_ch);
  }

  char const* what;
  switch (mvincch(delta->y, delta->x))
  {
    case SHADOW: case HWALL: case VWALL: what = "wall"; break;
    default: what = "air"; break;
  }

  msg("you swing at the %s", what);
  return true;
}

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
  coord const* dir = get_dir();
  if (dir == NULL)
    return false;

  coord delta = {
    .y = player_y() + dir->y,
    .x = player_x() + dir->x
  };

  THING* weapon = pack_equipped_item(EQUIPMENT_RHAND);

  return weapon != NULL && weapon->o_which == BOW
    ? command_attack_bow(dir)
    : command_attack_melee(fight_to_death, &delta);
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
    msg("'%s': %s", unctrl((chtype)ch), monster_name_by_type((char)ch));
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
    {',',	"	pick something up",			true},
    {'.',	"	rest for a turn",			true},
    {'/',	"	identify object",			true},
    {'<',	"	go up a staircase",			true},
    {'>',	"	go down a staircase",			true},
    {'?',	"	prints help",				true},
    {'A',	"	attack till either of you dies",	true},
    {'B',	"	run down & left",			false},
    {'D',	"	recall what's been discovered",		true},
    {'H',	"	run left",				false},
    {'I',	"	equipment",				true},
    {'J',	"	run down",				false},
    {'K',	"	run up",				false},
    {'L',	"	run right",				false},
    {'N',	"	run down & right",			false},
    {'P',	"	put on ring",				true},
    {'Q',	"	quit",					true},
    {'R',	"	remove ring",				true},
    {'S',	"	save game",				true},
    {'T',	"	take armor off",			true},
    {'U',	"	run up & right",			false},
    {'W',	"	wear armor",				true},
    {'Y',	"	run up & left",				false},
    {'Z',	"	rest until healed",			true},
    {'\0',	"	<CTRL><dir>: run till adjacent",	true},
    {'\0',	"	<SHIFT><dir>: run that way",		true},
    {'^',	"	identify trap type",			true},
    {'a',	"	attack in a direction",			true},
    {'b',	"	down & left",				true},
    {'c',	"	call object",				true},
    {'d',	"	drop object",				true},
    {'e',	"	eat food",				true},
    {'h',	"	left",					true},
    {'i',	"	inventory",				true},
    {'j',	"	down",					true},
    {'k',	"	up",					true},
    {'l',	"	right",					true},
    {'n',	"	down & right",				true},
    {'o',	"	examine/set options",			true},
    {'q',	"	quaff potion",				true},
    {'r',	"	read scroll",				true},
    {'s',	"	search for trap/secret door",		true},
    {'t',	"	throw something",			true},
    {'u',	"	up & right",				true},
    {'w',	"	wield a weapon",			true},
    {'x',	"	wield last used weapon",		true},
    {'y',	"	up & left",				true},
    {'z',	"	zap a wand in a direction",		true},
    {CTRL('B'),	"	run down & left until adjacent",	false},
    {CTRL('H'),	"	run left until adjacent",		false},
    {CTRL('J'),	"	run down until adjacent",		false},
    {CTRL('K'),	"	run up until adjacent",			false},
    {CTRL('L'),	"	run right until adjacent",		false},
    {CTRL('N'),	"	run down & right until adjacent",	false},
    {CTRL('P'),	"	repeat last message",			true},
    {CTRL('R'),	"	redraw screen",				true},
    {CTRL('U'),	"	run up & right until adjacent",		false},
    {CTRL('Y'),	"	run up & left until adjacent",		false},
    {CTRL('Z'),	"	shell escape",				true},
    {KEY_ESCAPE,"	cancel command",			true},
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
  if (dir == NULL)
    return false;

  int ydelta = dir->y;
  int xdelta = dir->x;
  dir = NULL;

  THING* obj = pack_get_item("throw", 0);
  if (obj == NULL)
    return false;

  if (obj->o_type == ARMOR)
  {
    msg("you can't throw armor");
    return false;
  }

  obj = pack_remove(obj, true, false);
  do_motion(obj, ydelta, xdelta);
  THING* monster_at_pos = level_get_monster(obj->o_pos.y, obj->o_pos.x);

  /* Throwing an arrow always misses */
  if (obj->o_which == ARROW)
  {
    if (monster_at_pos && !to_death)
    {
      char buf[MAXSTR];
      fight_missile_miss(obj, monster_name(monster_at_pos, buf));
    }
    fall(obj, true);
    return true;
  }
  /* AHA! Here it has hit something.  If it is a wall or a door,
   * or if it misses (combat) the monster, put it on the floor */
  else if (monster_at_pos == NULL ||
      !fight_against_monster(&obj->o_pos, obj, true))
    fall(obj, true);

  return true;


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
    if (monster_seen_by_player(mon))
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
  player_alerted = false;
  while (!player_alerted && player_is_hurt())
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

  food_eat();

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
    clearmsg();
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

