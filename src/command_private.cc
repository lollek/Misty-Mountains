#include <signal.h>

#include <string>

#include "shop.h"
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
#include "player.h"
#include "potions.h"
#include "rings.h"
#include "rogue.h"
#include "score.h"
#include "scrolls.h"
#include "traps.h"
#include "wand.h"
#include "weapons.h"
#include "gold.h"

#include "command_private.h"

using namespace std;

static bool command_attack_bow(Coordinate const* delta) {
  class Weapon* ptr = player->pack_find_ammo(player->equipped_weapon()->get_ammo_used());

  if (ptr == nullptr) {
    Game::io->message("you've run out of ammunition!");
    return true;
  }

  Item* arrow = player->pack_remove(ptr, true, false);
  Game::io->missile_motion(arrow, delta->y, delta->x);
  Monster* monster_at_pos = Game::level->get_monster(arrow->get_position());

  if (monster_at_pos == nullptr ||
      !fight_against_monster(&arrow->get_position(), arrow, true)) {
    weapon_missile_fall(arrow, true);
  }

  return true;
}

static bool command_attack_melee(bool fight_to_death, Coordinate const& delta) {
  Monster* mp = Game::level->get_monster(delta);
  if (mp != nullptr) {
    if (fight_to_death) {
      to_death = true;
      mp->set_players_target();
    }
    runch = dir_ch;
    return command_do(dir_ch);
  }

  string msg;
  switch (Game::level->get_tile(delta)) {
    case Tile::Wall:       msg = "you swing at the wall"; break;
    case Tile::ClosedDoor: msg = "you swing at the door"; break;
    case Tile::Stairs:     msg = "you swing at the stairs"; break;

    case Tile::OpenDoor:
    case Tile::Floor:
    case Tile::Trap:   msg = "you swing at the air"; break;
  }

  Game::io->message(msg);
  return true;
}

bool command_use_stairs(char up_or_down) {
  if (up_or_down != '>' && up_or_down != '<') {
    error("Unknown stairs: " + to_string(up_or_down));
  }

  if (player->is_levitating()) {
    Game::io->message("You can't. You're floating off the ground!");
  } else if (Game::level->get_tile(player->get_position()) != Tile::Stairs) {
    Game::io->message("You're not standing on any stairs");
  }

  // DOWN
  else if (up_or_down == '>') {
    Game::new_level(Game::current_level +1);

  // UP
  } else if (up_or_down == '<') {
    bool has_amulet = player->pack_contains_amulet();

    if (Game::current_level < 0) {
      error("Level should not go lower than 0");
    }

    if (Game::current_level == 1) {
      if (has_amulet) {
        score_win_and_exit();
      } else {
        if (Game::level->shop == nullptr) {
          Game::level->shop = new Shop();
        }
        Game::level->shop->enter();
        return false;
      }
    }

    Game::new_level(Game::current_level -1);

    if (has_amulet) {
      Game::io->message("you feel a wrenching sensation in your gut");
    }
  }

  return false;
}

bool command_attack(bool fight_to_death) {
  Coordinate const* dir = get_dir();
  if (dir == nullptr) {
    return false;
  }

  Coordinate delta = player->get_position();
  delta.x += dir->x;
  delta.y += dir->y;

  class Weapon* weapon = player->equipped_weapon();

  return weapon != nullptr && weapon->is_missile_launcher()
    ? command_attack_bow(dir)
    : command_attack_melee(fight_to_death, delta);
}

bool command_inscribe_item() {
  Item* obj = player->pack_find_item("inscribe", 0);
  if (obj == nullptr) {
    return false;
  }

  string* guess = nullptr;
  switch (obj->o_type) {
    case IO::Amulet: Game::io->message("Cannot inscribe the amulet"); return false;
    case IO::Food: Game::io->message("Don't play with your food!"); return false;

    case IO::Ring:
      guess = &Ring::guess(static_cast<Ring::Type>(obj->o_which));
      break;

    case IO::Potion:
      guess = &Potion::guess(static_cast<Potion::Type>(obj->o_which));
      break;

    case IO::Scroll:
      guess = &Scroll::guess(static_cast<Scroll::Type>(obj->o_which));
      break;

    case IO::Wand:
      guess = &Wand::guess(static_cast<Wand::Type>(obj->o_which));
      break;
  }

  Game::io->message("what do you want to inscribe on it?");

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

bool command_identify_trap() {
  const Coordinate* dir = get_dir();
  if (dir == nullptr) {
    return false;
  }

  Coordinate delta = player->get_position();
  delta.x += dir->x;
  delta.y += dir->y;

  if (Game::level->get_tile(delta) != Tile::Trap) {
    Game::io->message("no trap there");
  } else if (player->has_confusing_attack()) {
    Trap::Type rand_trap = static_cast<Trap::Type>(os_rand_range(static_cast<size_t>(Trap::NTRAPS)));
    Game::io->message(Trap::name(rand_trap));
  } else {
    Game::io->message(Trap::name(static_cast<Trap::Type>(Game::level->get_trap_type(delta))));
    Game::level->set_discovered(delta);
  }
  return false;
}

bool command_quit() {
  command_signal_quit(0);
  return false;
}

bool command_pick_up(bool force) {
  if (player->is_levitating()) {
    if (force) {
      Game::io->message("You can't. You're floating off the ground!");
    }
    return false;
  }


  // Collect all items which are at this location
  Coordinate const& coord = player->get_position();
  list<Item*> items_here;
  for (Item* item : Game::level->items) {
    if (item->get_position() == coord) {
      items_here.push_back(item);

      // If the item was of someone's desire, they will get mad and attack
      monster_aggro_all_which_desire_item(item);
    }
  }

  if (force && items_here.empty()) {
    Game::io->message("nothing to pick up");
    return false;
  }

  // No iterator in this loop, so we can delete while looping
  auto it = items_here.begin();
  while (it != items_here.end()) {
    Item* obj = *it;
    switch (obj->o_type) {

      case IO::Gold: {
        Gold* gold = dynamic_cast<Gold*>(obj);
        if (gold == nullptr) {
          error("casted gold to Gold* which became null");
        }

        int value = gold->get_amount();
        if (value > 0) {
          Game::io->message("you found " + to_string(value) + " gold pieces");
        }

        player->give_gold(value);
        Game::level->items.remove(obj);

        delete obj;
        it = items_here.erase(it);
      } break;

      case IO::Potion: case IO::Weapon: case IO::Ammo: case IO::Food: case IO::Armor:
      case IO::Scroll: case IO::Amulet: case IO::Ring: case IO::Wand: {
        if (force || obj->autopickup()) {
          player->pack_add(obj, false, true);
          it = items_here.erase(it);
        } else {
          ++it;
        }
      } break;

      default: {
        error("Unknown type to pick up");
      }
    }
  }

  if (!items_here.empty()) {
    stringstream os;
    os << "items here: ";
    for (Item* item : items_here) {
      os << item->get_description();
      if (item != items_here.back()) {
        os << ", ";
      }
    }
    Game::io->message(os.str());
  }

  return true;
}

bool command_help() {
  struct list {
    char sym;
    string description;
    bool print;
  } const helpstr[] = {
    {',',	"	pick something up",			true},
    {'.',	"	rest for a turn",			true},
    {'<',	"	go up a staircase",			true},
    {'>',	"	go down a staircase",			true},
    {'?',	"	prints help",				true},
    {'{',	"	inscribe something",			true},
    {'A',	"	attack till either of you dies",	true},
    {'B',	"	run down & left",			false},
    {'E',	"	equipment",				true},
    {'H',	"	run left",				false},
    {'I',	"	inventory",				true},
    {'J',	"	run down",				false},
    {'K',	"	run up",				false},
    {'L',	"	run right",				false},
    {'N',	"	run down & right",			false},
    {'Q',	"	quit",					true},
    {'S',	"	save game",				true},
    {'U',	"	run up & right",			false},
    {'Y',	"	run up & left",				false},
    {'Z',	"	rest until healed",			true},
    {'\0',	"	<CTRL><dir>: run till adjacent",	true},
    {'\0',	"	<SHIFT><dir>: run that way",		true},
    {'^',	"	identify trap type",			true},
    {'a',	"	attack in a direction",			true},
    {'b',	"	down & left",				true},
    {'e',	"	eat food",				true},
    {'h',	"	left",					true},
    {'j',	"	down",					true},
    {'k',	"	up",					true},
    {'l',	"	right",					true},
    {'n',	"	down & right",				true},
    {'O',	"	examine/set options",			true},
    {'q',	"	quaff potion",				true},
    {'r',	"	read scroll",				true},
    {'s',	"	search for trap/secret door",		true},
    {'t',	"	throw something",			true},
    {'u',	"	up & right",				true},
    {'x',	"	swap weapons",				true},
    {'y',	"	up & left",				true},
    {'z',	"	zap a wand in a direction",		true},
    {CTRL('B'),	"	run down & left until adjacent",	false},
    {CTRL('H'),	"	run left until adjacent",		false},
    {CTRL('J'),	"	run down until adjacent",		false},
    {CTRL('K'),	"	run up until adjacent",			false},
    {CTRL('L'),	"	run right until adjacent",		false},
    {CTRL('N'),	"	run down & right until adjacent",	false},
    {CTRL('P'),	"	show previous messages",		true},
    {CTRL('R'),	"	redraw screen",				true},
    {CTRL('U'),	"	run up & right until adjacent",		false},
    {CTRL('Y'),	"	run up & left until adjacent",		false},
    {CTRL('Z'),	"	shell escape",				true},
    {KEY_ESCAPE,"	cancel command",			true},
  };
  int const helpstrsize = sizeof(helpstr) / sizeof(*helpstr);

  Game::io->message("character you want help for (* for all): ");
  char helpch = Game::io->readchar(true);
  Game::io->clear_message();

  /* If its not a *, print the right help string
   * or an error if he typed a funny character. */
  if (helpch != '*') {
    move(0, 0);
    for (int i = 0; i < helpstrsize; ++i) {
      if (helpstr[i].sym == helpch) {
        Game::io->message(string(1, UNCTRL(helpstr[i].sym)) + ")" + 
                          helpstr[i].description);
        return false;
      }
    }
    Game::io->message("unknown character '" + string(1, helpch) + "'");
    return false;
  }

  /* Here we print help for everything.
   * Then wait before we return to command mode */

  int numprint = 0;
  for (int i = 0; i < helpstrsize; ++i) {
    if (helpstr[i].print) {
      ++numprint;
    }
  }

  numprint /= 2;
  if (numprint > LINES - 1) {
    numprint = LINES - 1;
  }

  wclear(Game::io->extra_screen);
  int print_i = 0;
  for (int i = 0; i < helpstrsize; ++i) {
    if (!helpstr[i].print) {
      continue;
    }

    wmove(Game::io->extra_screen, print_i % numprint, print_i >= numprint ? COLS / 2 : 0);
    if (helpstr[i].sym) {
      waddstr(Game::io->extra_screen, unctrl(static_cast<chtype>(helpstr[i].sym)));
    }
    waddstr(Game::io->extra_screen, helpstr[i].description.c_str());

    if (++print_i >= numprint * 2) {
      break;
    }
  }

  wmove(Game::io->extra_screen, LINES - 1, 0);
  waddstr(Game::io->extra_screen, "--Press space to continue--");
  wrefresh(Game::io->extra_screen);
  Game::io->wait_for_key(KEY_SPACE);
  clearok(stdscr, true);

  Game::io->clear_message();
  touchwin(stdscr);
  wrefresh(stdscr);
  return false;
}

/* Let them escape for a while */
void command_shell() {
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

bool command_throw() {
  const Coordinate* dir = get_dir();
  if (dir == nullptr) {
    return false;
  }

  int ydelta = dir->y;
  int xdelta = dir->x;
  dir = nullptr;

  Item* obj = player->pack_find_item("throw", 0);
  if (obj == nullptr) {
    return false;
  }

  if (obj->o_type == IO::Armor) {
    Game::io->message("you can't throw armor");
    return false;
  }

  obj = player->pack_remove(obj, true, false);
  Game::io->missile_motion(obj, ydelta, xdelta);
  Monster* monster_at_pos = Game::level->get_monster(obj->get_position());

  /* Throwing an arrow always misses */
  if (obj->o_which == Weapon::Arrow) {
    if (monster_at_pos && !to_death) {
      fight_missile_miss(obj, monster_at_pos->get_name().c_str());
    }
    weapon_missile_fall(obj, true);
    return true;
  }

  /* AHA! Here it has hit something.  If it is a wall or a door,
   * or if it misses (combat) the monster, put it on the floor */
  bool missed = monster_at_pos == nullptr ||
    !fight_against_monster(&obj->get_position(), obj, true);

  if (missed) {
    if (obj->o_type == IO::Potion) {
      Game::io->message("the potion crashes into the wall");
    } else {
      weapon_missile_fall(obj, true);
    }
  }

  return true;


}

bool command_rest() {
  if (monster_is_anyone_seen_by_player()) {
    Game::io->message("cannot rest with monsters nearby");
    return false;
  }

  if (!player->is_hurt()) {
    Game::io->message("you don't feel the least bit tired");
    return false;
  }

  Game::io->message("you rest for a while");
  player_alerted = false;
  while (!player_alerted && player->is_hurt()) {
    Daemons::daemon_run_before();
    Daemons::daemon_run_after();
  }
  return true;
}

bool command_eat() {
  Item* obj = player->pack_find_item("eat", IO::Food);
  if (obj == nullptr) {
    return false;
  }

  if (obj->o_type != IO::Food) {
    Game::io->message("that's inedible!");
    return false;
  }

  player->pack_remove(obj, false, false);
  Food* food = dynamic_cast<Food*>(obj);
  if (food == nullptr) {
    error("Error casting food");
  }
  player->eat(food);
  return true;
}

bool command_run(char ch, bool cautiously) {
  runch = ch;

  if (player->is_blind()) {
    Game::io->message("You stumble forward");
  } else {
    player->set_running();
  }

  if (cautiously) {
    ch = UNCTRL(ch);
  }

  return move_do(ch, cautiously);
}

bool command_read_scroll() {

  Item* obj = player->pack_find_item("read", IO::Scroll);
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
  player->pack_remove(scroll, false, false);

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

bool command_open() {
  const Coordinate* dir = get_dir();
  if (dir == nullptr) {
    return false;
  }

  Coordinate const& player_pos = player->get_position();
  Tile::Type door = Game::level->get_tile(player_pos.x + dir->x, player_pos.y + dir->y);
  if (door == Tile::ClosedDoor) {
    Game::level->set_tile(player_pos.x + dir->x, player_pos.y + dir->y, Tile::OpenDoor);
    return true;
  } else {
    Game::io->message("Nothing to open there");
    return false;
  }
}

bool command_close() {
  const Coordinate* dir{get_dir()};
  if (dir == nullptr) {
    return false;
  }

  Coordinate const& player_pos{player->get_position()};
  Coordinate door_coord{player_pos.x + dir->x, player_pos.y + dir->y};
  Tile::Type door{Game::level->get_tile(door_coord)};
  if (door != Tile::OpenDoor) {
    Game::io->message("Nothing to close there");
    return false;
  }

  Monster* monster = Game::level->get_monster(door_coord);
  if (monster != nullptr) {
    Game::io->message(monster->get_name() + " is blocking the doorway");
    return false;
  }

  Game::level->set_tile(door_coord, Tile::ClosedDoor);
  return true;
}

bool command_save() {
  Game::io->message("really save and exit?");

  if (Game::io->readchar(true) == 'y') {
    if (Game::save()) {
      Game::exit();
    }
  }

  return false;
}

