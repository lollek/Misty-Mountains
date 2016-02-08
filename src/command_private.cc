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
    Game::io->message("you've run out of arrows!");
    return true;
  }

  Item* arrow = pack_remove(ptr, true, false);
  io_missile_motion(arrow, delta->y, delta->x);
  Monster* monster_at_pos = Game::level->get_monster(arrow->get_position());

  if (monster_at_pos == nullptr || !fight_against_monster(&arrow->get_position(), arrow, true))
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

  string msg;
  switch (Game::level->get_tile(delta))
  {
    case Tile::Shadow:
    case Tile::Wall:   msg = "you swing at the wall"; break;

    case Tile::Stairs: msg = "you swing at the stairs"; break;

    case Tile::Door:
    case Tile::Floor:
    case Tile::Trap:   msg = "you swing at the air"; break;
  }

  Game::io->message(msg);
  return true;
}

bool
command_use_stairs(char up_or_down)
{
  if (up_or_down != '>' && up_or_down != '<') {
    error("Unknown stairs: " + to_string(up_or_down));
  }

  if (player->is_levitating())
    Game::io->message("You can't. You're floating off the ground!");
  else if (Game::level->get_tile(player->get_position()) != Tile::Stairs)
    Game::io->message("You're not standing on any stairs");

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
      Game::io->message("you feel a wrenching sensation in your gut");
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

  return weapon != nullptr && weapon->o_which == Weapon::BOW
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
    case IO::Food: Game::io->message("Don't play with your food!"); return false;

    case IO::Ring:
      already_known = Ring::is_known(static_cast<Ring::Type>(obj->o_which));
      guess = &Ring::guess(static_cast<Ring::Type>(obj->o_which));
      break;

    case IO::Potion:
      already_known = Potion::is_known(static_cast<Potion::Type>(obj->o_which));
      guess = &Potion::guess(static_cast<Potion::Type>(obj->o_which));
      break;

    case IO::Scroll:
      already_known = Scroll::is_known(static_cast<Scroll::Type>(obj->o_which));
      guess = &Scroll::guess(static_cast<Scroll::Type>(obj->o_which));
      break;

    case IO::Wand:
      already_known = Wand::is_known(static_cast<Wand::Type>(obj->o_which));
      guess = &Wand::guess(static_cast<Wand::Type>(obj->o_which));
      break;

    default:
      already_known = false;
      break;
  }

  if (already_known)
  {
    Game::io->message("that has already been identified");
    return false;
  }

  Game::io->message("what do you want to call it?");

  string new_name = Game::io->read_string();
  if (!new_name.empty()) {
    if (guess != nullptr) {
      *guess = new_name;
    } else {
      obj->set_nickname(new_name);
    }
  }

  Game::io->clear_message();
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

  if (Game::level->get_tile(delta) != Tile::Trap) {
    Game::io->message("no trap there");
  } else if (player->has_confusing_attack()) {
    Trap::Type rand_trap = static_cast<Trap::Type>(os_rand_range(static_cast<size_t>(Trap::NTRAPS)));
    Game::io->message(Trap::name(rand_trap));
  } else
  {
    Game::io->message(Trap::name(static_cast<Trap::Type>(Game::level->get_trap_type(delta))));
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
    Game::io->message("You can't. You're floating off the ground!");
  }

  Coordinate const& coord = player->get_position();
  if (Game::level->get_item(coord) == nullptr) {
    Game::io->message("nothing to pick up");
    return false;
  }

  pack_pick_up(coord, true);
  return true;
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

  Game::io->message("character you want help for (* for all): ");
  char helpch = io_readchar(true);
  Game::io->clear_message();

  /* If its not a *, print the right help string
   * or an error if he typed a funny character. */
  if (helpch != '*')
  {
    move(0, 0);
    for (int i = 0; i < helpstrsize; ++i)
      if (helpstr[i].sym == helpch)
      {
        Game::io->message(string(1, UNCTRL(helpstr[i].sym)) + ")" + 
                          helpstr[i].description);
        return false;
      }
    Game::io->message("unknown character '" + string(1, UNCTRL(helpch)) + "'");
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

  wclear(Game::io->extra_screen);
  int print_i = 0;
  for (int i = 0; i < helpstrsize; ++i)
  {
    if (!helpstr[i].print)
      continue;

    wmove(Game::io->extra_screen, print_i % numprint, print_i >= numprint ? COLS / 2 : 0);
    if (helpstr[i].sym)
      waddstr(Game::io->extra_screen, unctrl(static_cast<chtype>(helpstr[i].sym)));
    waddstr(Game::io->extra_screen, helpstr[i].description.c_str());

    if (++print_i >= numprint * 2)
      break;
  }

  wmove(Game::io->extra_screen, LINES - 1, 0);
  waddstr(Game::io->extra_screen, "--Press space to continue--");
  wrefresh(Game::io->extra_screen);
  io_wait_for_key(KEY_SPACE);
  clearok(stdscr, true);

  Game::io->clear_message();
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
    Game::io->message("You inventory is empty");
    return false;
  }

  pack_print_inventory(0);
  Game::io->message("--Press any key to continue--");
  io_readchar(false);
  pack_clear_inventory();
  Game::io->clear_message();
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

  if (obj->o_type == IO::Armor)
  {
    Game::io->message("you can't throw armor");
    return false;
  }

  obj = pack_remove(obj, true, false);
  io_missile_motion(obj, ydelta, xdelta);
  Monster* monster_at_pos = Game::level->get_monster(obj->get_position());

  /* Throwing an arrow always misses */
  if (obj->o_which == Weapon::ARROW)
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
    !fight_against_monster(&obj->get_position(), obj, true);

  if (missed)
  {
    if (obj->o_type == IO::Potion)
      Game::io->message("the potion crashes into the wall");
    else
      weapon_missile_fall(obj, true);
  }

  return true;


}

bool
command_wield(void)
{
  Item* obj = pack_get_item("wield", IO::Weapon);

  if (obj == nullptr)
    return false;

  if (obj->o_type == IO::Armor)
  {
    Game::io->message("you can't wield armor");
    return command_wield();
  }

  return command_weapon_wield(obj);
}

bool command_rest(void)
{
  if (monster_is_anyone_seen_by_player())
  {
    Game::io->message("cannot rest with monsters nearby");
    return false;
  }

  if (!player->is_hurt())
  {
    Game::io->message("you don't feel the least bit tired");
    return false;
  }

  Game::io->message("you rest for a while");
  player_alerted = false;
  while (!player_alerted && player->is_hurt())
  {
    Daemons::daemon_run_before();
    Daemons::daemon_run_after();
  }
  return true;
}

bool
command_eat(void)
{
  Item* obj = pack_get_item("eat", IO::Food);
  if (obj == nullptr)
    return false;

  if (obj->o_type != IO::Food)
  {
    Game::io->message("that's inedible!");
    return false;
  }

  player->eat();

  if (obj->o_which == 1)
    Game::io->message("my, that was a yummy fruit");

  else if (os_rand_range(100) > 70)
  {
    player->gain_experience(1);
    Game::io->message("this food tastes awful");
    player->check_for_level_up();
  }

  else
    Game::io->message("that tasted good");

  pack_remove(obj, false, false);
  return true;
}

bool
command_run(char ch, bool cautiously)
{
  if (player->is_blind()) {
    Game::io->message("You stumble forward");
  } else {
    if (cautiously) {
      door_stop = true;
    }
    player->set_running();
  }

  runch = static_cast<char>(tolower(ch));
  return move_do(runch);
}

bool command_drop(void)
{
  Item* obj = pack_get_item("drop", 0);
  if (obj == nullptr)
    return false;

  bool drop_all = false;
  if (obj->o_count > 1)
  {
    Game::io->message("Drop all? (y/N) ");
    drop_all = io_readchar(true) == 'y';
    Game::io->clear_message();
  }

  obj = pack_remove(obj, true, drop_all);

  /* Link it into the level object list */
  Game::level->items.push_back(obj);

  obj->set_position(player->get_position());

  Game::io->message("dropped " + obj->get_description());
  return true;
}

bool command_wear() {
  Item* obj = pack_get_item("wear", IO::Armor);

  if (obj == nullptr) {
    return false;
  }

  if (obj->o_type != IO::Armor) {
    Game::io->message("you can't wear that");
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

  Game::io->message("now wearing " + obj->get_description());
  return true;
}

bool
command_ring_put_on(void)
{
  Item* obj = pack_get_item("put on", IO::Ring);

  /* Make certain that it is somethings that we want to wear */
  if (obj == nullptr)
    return false;

  Ring* ring = dynamic_cast<Ring*>(obj);
  if (obj->o_type != IO::Ring || ring == nullptr)
  {
    Game::io->message("not a ring");
    return command_ring_put_on();
  }

  /* Try to put it on */
  if (!pack_equip_item(obj))
  {
    Game::io->message("you already have a ring on each hand");
    return false;
  }
  pack_remove(obj, false, true);

  /* Calculate the effect it has on the poor guy. */
  switch (obj->o_which)
  {
    case Ring::AGGR: monster_aggravate_all(); break;
  }

  string msg = ring->get_description();
  msg.at(0) = static_cast<char>(tolower(msg.at(0)));
  Game::io->message("now wearing " + msg);
  return true;
}

bool
command_ring_take_off(void)
{
  enum equipment_pos ring;

  /* Try right, then left */
  if (pack_equipped_item(EQUIPMENT_RRING) != nullptr)
    ring = EQUIPMENT_RRING;
  else
    ring = EQUIPMENT_LRING;

  Item* obj = pack_equipped_item(ring);

  if (!pack_unequip(ring, false))
    return false;

  switch (obj->o_which)
  {
    case Ring::ADDSTR:
      break;

    case Ring::SEEINVIS:
      Daemons::daemon_extinguish_fuse(Daemons::daemon_function::remove_true_sight);
      break;
  }
  return true;
}

bool
command_read_scroll() {

  Item* obj = pack_get_item("read", IO::Scroll);
  if (obj == nullptr) {
    return false;
  }

  Scroll* scroll = dynamic_cast<Scroll*>(obj);
  if (obj->o_type != IO::Scroll || scroll == nullptr) {
    Game::io->message("there is nothing on it to read");
    return false;
  }

  /* Get rid of the thing */
  bool discardit = scroll->o_count == 1;
  pack_remove(scroll, false, false);

  Scroll::Type subtype = scroll->get_type();
  bool was_known = Scroll::is_known(subtype);

  scroll->read();

  if (!was_known) {
    string& nickname = Scroll::guess(scroll->get_type());
    if (Scroll::is_known(subtype)) {
      nickname.clear();

    } else if (nickname.empty()) {
      Game::io->message("what do you want to call the scroll?");
      nickname = Game::io->read_string();
    }
  }

  if (discardit) {
    delete obj;
  }

  return true;

}

static Item* last_wielded_weapon = nullptr;

bool command_weapon_wield(Item* weapon) {
  Item* currently_wielding = pack_equipped_item(EQUIPMENT_RHAND);
  if (currently_wielding != nullptr) {
    if (!pack_unequip(EQUIPMENT_RHAND, true)) {
      return true;
    }
  }

  pack_remove(weapon, false, true);
  pack_equip_item(weapon);

  Game::io->message("wielding " + weapon->get_description());
  last_wielded_weapon = currently_wielding;
  return true;
}

void command_weapon_set_last_used(Item* weapon) {
  last_wielded_weapon = weapon;
}

bool command_weapon_wield_last_used(void) {

  if (last_wielded_weapon == nullptr || !pack_contains(last_wielded_weapon)) {
    last_wielded_weapon = nullptr;
    Game::io->message("you have no weapon to switch to");
    return false;
  }

  return command_weapon_wield(last_wielded_weapon);
}




