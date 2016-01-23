#include <signal.h>

#include <string>

using namespace std;

#include "error_handling.h"
#include "game.h"
#include "coordinate.h"
#include "daemons.h"
#include "fight.h"
#include "food.h"
#include "io.h"
#include "level.h"
#include "misc.h"
#include "monster.h"
#include "move.h"
#include "options.h"
#include "os.h"
#include "pack.h"
#include "player.h"
#include "potions.h"
#include "rings.h"
#include "rogue.h"
#include "score.h"
#include "scrolls.h"
#include "traps.h"
#include "wand.h"
#include "weapons.h"

#include "command_private.h"

static bool command_attack_bow(Coordinate const* delta)
{
  Item* ptr = pack_find_arrow();

  if (ptr == nullptr)
  {
    io_msg("you've run out of arrows!");
    return true;
  }

  Item* arrow = pack_remove(ptr, true, false);
  io_missile_motion(arrow, delta->y, delta->x);
  Monster* monster_at_pos = Game::level->get_monster(arrow->get_pos());

  if (monster_at_pos == nullptr || !fight_against_monster(&arrow->get_pos(), arrow, true))
    weapon_missile_fall(arrow, true);

  return true;
}

static bool command_attack_melee(bool fight_to_death, Coordinate const& delta)
{
  Monster* mp = Game::level->get_monster(delta);
  if (mp != nullptr && diag_ok(&player->get_position(), &delta))
  {
    if (fight_to_death)
    {
      to_death = true;
      mp->set_players_target();
    }
    runch = dir_ch;
    return command_do(dir_ch);
  }

  char const* what;
  switch (mvincch(delta.y, delta.x))
  {
    case SHADOW: case HWALL: case VWALL: what = "wall"; break;
    default: what = "air"; break;
  }

  io_msg("you swing at the %s", what);
  return true;
}

bool
command_use_stairs(char up_or_down)
{
  if (up_or_down != '>' && up_or_down != '<') {
    error("Unknown stairs: " + to_string(up_or_down));
  }

  if (player->is_levitating())
    io_msg("You can't. You're floating off the ground!");
  else if (Game::level->get_ch(player->get_position()) != STAIRS)
    io_msg("You're not standing on any stairs");

  else if (up_or_down == '>') /* DOWN */ {
    Game::new_level(Game::current_level +1);
  }

  else if (up_or_down == '<') /* UP */
  {
    bool has_amulet = pack_contains_amulet();

    if (Game::current_level < 0) {
      error("Level should not go lower than 0");
    }

    if (Game::current_level == 1)
    {
      if (has_amulet)
        score_win_and_exit();
      else
      {
        /* This either quits, or player did not want to leave */
        command_quit();
        return false;
      }
    }

    Game::new_level(Game::current_level -1);

    if (has_amulet)
      io_msg("you feel a wrenching sensation in your gut");
  }

  return false;
}

bool
command_attack(bool fight_to_death)
{
  Coordinate const* dir = get_dir();
  if (dir == nullptr)
    return false;

  Coordinate delta = player->get_position();
  delta.x += dir->x;
  delta.y += dir->y;

  Item* weapon = pack_equipped_item(EQUIPMENT_RHAND);

  return weapon != nullptr && weapon->o_which == BOW
    ? command_attack_bow(dir)
    : command_attack_melee(fight_to_death, delta);
}

bool
command_name_item(void)
{
  Item* obj = pack_get_item("rename", PACK_RENAMEABLE);

  if (obj == nullptr)
    return false;

  bool already_known;
  string* guess = nullptr;
  switch (obj->o_type)
  {
    case FOOD: io_msg("Don't play with your food!"); return false;

    case RING:
      already_known = ring_is_known(static_cast<ring_t>(obj->o_which));
      guess = &ring_info[static_cast<size_t>(obj->o_which)].oi_guess;
      break;

    case POTION:
      already_known = Potion::is_known(static_cast<Potion::Type>(obj->o_which));
      guess = &Potion::guess(static_cast<Potion::Type>(obj->o_which));
      break;

    case SCROLL:
      already_known = scroll_is_known(static_cast<scroll_t>(obj->o_which));
      break;

    case STICK:
      already_known = Wand::is_known(static_cast<Wand::Type>(obj->o_which));
      guess = &Wand::guess(static_cast<Wand::Type>(obj->o_which));
      break;

    default:
      already_known = false;
      break;
  }

  if (already_known)
  {
    io_msg("that has already been identified");
    return false;
  }

  io_msg("what do you want to call it? ");

  char tmpbuf[MAXSTR] = { '\0' };
  if (io_readstr(tmpbuf) == 0)
  {
    if (obj->o_type == SCROLL) {
      scroll_set_name(static_cast<scroll_t>(obj->o_which), tmpbuf);
    } else if (strlen(tmpbuf) > 0) {
      if (guess != nullptr) {
        *guess = tmpbuf;
      } else {
        obj->set_nickname(tmpbuf);
      }
    }
  }

  io_msg_clear();
  return false;
}

bool
command_identify_character(void)
{
  io_msg("what do you want identified? ");
  int ch = io_readchar(true);
  io_msg_clear();

  if (ch == KEY_ESCAPE)
  {
    io_msg_clear();
    return false;
  }

  if (isalpha(ch))
  {
    ch = toupper(ch);
    io_msg("'%s': %s", unctrl(static_cast<chtype>(ch)), monster_name_by_type(static_cast<char>(ch)).c_str());
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
      io_msg("'%s': %s", unctrl(static_cast<chtype>(ch)), ptr->description);
      return false;
    }

  io_msg("'%s': %s", unctrl(static_cast<chtype>(ch)), "unknown character");
  return false;
}

bool
command_identify_trap(void)
{
  const Coordinate* dir = get_dir();
  if (dir == nullptr)
    return false;

  Coordinate delta = player->get_position();
  delta.x += dir->x;
  delta.y += dir->y;

  if (Game::level->get_ch(delta) != TRAP)
    io_msg("no trap there");
  else if (player->has_confusing_attack())
    io_msg(trap_names[os_rand_range(NTRAPS)].c_str());
  else
  {
    io_msg(trap_names[Game::level->get_trap_type(delta)].c_str());
    Game::level->set_discovered(delta);
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
command_pick_up(void) {
  if (player->is_levitating()) {
    io_msg("You can't. You're floating off the ground!");
  }

  for (Item* obj : Game::level->items) {
    if (obj->get_pos() == player->get_position()) {
      pack_pick_up(obj, true);
      return true;
    }
  }

  io_msg("nothing to pick up");
  return false;
}

bool
command_help(void)
{
  struct list
  {
    char sym;
    string description;
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

  io_msg("character you want help for (* for all): ");
  char helpch = io_readchar(true);
  io_msg_clear();

  /* If its not a *, print the right help string
   * or an error if he typed a funny character. */
  if (helpch != '*')
  {
    move(0, 0);
    for (int i = 0; i < helpstrsize; ++i)
      if (helpstr[i].sym == helpch)
      {
        io_msg("%s)%s", unctrl(static_cast<chtype>(helpstr[i].sym)), 
               helpstr[i].description.c_str());
        return false;
      }
    io_msg("unknown character '%s'", unctrl(static_cast<chtype>(helpch)));
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
      waddstr(hw, unctrl(static_cast<chtype>(helpstr[i].sym)));
    waddstr(hw, helpstr[i].description.c_str());

    if (++print_i >= numprint * 2)
      break;
  }

  wmove(hw, LINES - 1, 0);
  waddstr(hw, "--Press space to continue--");
  wrefresh(hw);
  io_wait_for_key(KEY_SPACE);
  clearok(stdscr, true);

  io_msg_clear();
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
    io_msg("You inventory is empty");
    return false;
  }

  pack_print_inventory(0);
  io_msg("--Press any key to continue--");
  io_readchar(false);
  pack_clear_inventory();
  io_msg_clear();
  return false;
}

bool
command_take_off(enum equipment_pos pos)
{
  if (pack_equipped_item(pos) == nullptr)
    return false;

  pack_unequip(pos, false);
  return pack_equipped_item(pos) != nullptr;
}

bool command_throw(void)
{
  const Coordinate* dir = get_dir();
  if (dir == nullptr)
    return false;

  int ydelta = dir->y;
  int xdelta = dir->x;
  dir = nullptr;

  Item* obj = pack_get_item("throw", 0);
  if (obj == nullptr)
    return false;

  if (obj->o_type == ARMOR)
  {
    io_msg("you can't throw armor");
    return false;
  }

  obj = pack_remove(obj, true, false);
  io_missile_motion(obj, ydelta, xdelta);
  Monster* monster_at_pos = Game::level->get_monster(obj->get_pos());

  /* Throwing an arrow always misses */
  if (obj->o_which == ARROW)
  {
    if (monster_at_pos && !to_death)
    {
      fight_missile_miss(obj, monster_at_pos->get_name().c_str());
    }
    weapon_missile_fall(obj, true);
    return true;
  }

  /* AHA! Here it has hit something.  If it is a wall or a door,
   * or if it misses (combat) the monster, put it on the floor */
  bool missed = monster_at_pos == nullptr ||
    !fight_against_monster(&obj->get_pos(), obj, true);

  if (missed)
  {
    if (obj->o_type == POTION)
      io_msg("the potion crashes into the wall");
    else
      weapon_missile_fall(obj, true);
  }

  return true;


}

bool
command_wield(void)
{
  Item* obj = pack_get_item("wield", WEAPON);

  if (obj == nullptr)
    return false;

  if (obj->o_type == ARMOR)
  {
    io_msg("you can't wield armor");
    return command_wield();
  }

  return weapon_wield(obj);
}

bool command_rest(void)
{
  if (monster_is_anyone_seen_by_player())
  {
    io_msg("cannot rest with monsters nearby");
    return false;
  }

  if (!player->is_hurt())
  {
    io_msg("you don't feel the least bit tired");
    return false;
  }

  io_msg("you rest for a while");
  player_alerted = false;
  while (!player_alerted && player->is_hurt())
  {
    daemon_run_before();
    daemon_run_after();
  }
  return true;
}

bool
command_eat(void)
{
  Item* obj = pack_get_item("eat", FOOD);
  if (obj == nullptr)
    return false;

  if (obj->o_type != FOOD)
  {
    io_msg("that's inedible!");
    return false;
  }

  food_eat();

  if (obj->o_which == 1)
    io_msg("my, that was a yummy fruit");

  else if (os_rand_range(100) > 70)
  {
    player->gain_experience(1);
    io_msg("this food tastes awful");
    player->check_for_level_up();
  }

  else
    io_msg("that tasted good");

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

  player->set_running();
  runch = static_cast<char>(tolower(ch));
  return move_do(runch);
}

bool command_drop(void)
{
  char ch = Game::level->get_ch(player->get_position());

  if (ch != FLOOR && ch != PASSAGE)
  {
    io_msg("there is something there already");
    return false;
  }

  Item* obj = pack_get_item("drop", 0);
  if (obj == nullptr)
    return false;

  bool drop_all = false;
  if (obj->o_count > 1)
  {
    io_msg("Drop all? (y/N) ");
    drop_all = io_readchar(true) == 'y';
    io_msg_clear();
  }

  obj = pack_remove(obj, true, drop_all);

  /* Link it into the level object list */
  Game::level->items.push_back(obj);

  Game::level->set_ch(player->get_position(), static_cast<char>(obj->o_type));

  obj->set_pos(player->get_position());

  io_msg("dropped %s", inv_name(obj, true).c_str());
  return true;
}

bool command_wear() {
  Item* obj = pack_get_item("wear", ARMOR);

  if (obj == nullptr) {
    return false;
  }

  if (obj->o_type != ARMOR) {
    io_msg("you can't wear that");
    return command_wear();
  }

  if (pack_equipped_item(EQUIPMENT_ARMOR) != nullptr) {
    if (!pack_unequip(EQUIPMENT_ARMOR, false)) {
      return true;
    }
  }

  player->waste_time(1);
  pack_remove(obj, false, true);
  pack_equip_item(obj);

  io_msg("now wearing %s", inv_name(obj, true).c_str());
  return true;
}


