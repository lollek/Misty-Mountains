#include <list>

using namespace std;

#include "error_handling.h"
#include "game.h"
#include "traps.h"
#include "io.h"
#include "pack.h"
#include "daemons.h"
#include "monster.h"
#include "misc.h"
#include "player.h"
#include "level_rooms.h"
#include "things.h"
#include "os.h"
#include "rogue.h"

#include "level.h"

/* Tuneables */
#define MAXOBJ		9  /* How many attempts to put items in dungeon */
#define TREAS_ROOM	20 /* one chance in TREAS_ROOM for a treasure room */
#define MAXTREAS	10 /* maximum number of treasures in a treasure room */
#define MINTREAS	2  /* minimum number of treasures in a treasure room */
#define MAXTRIES	10 /* max number of tries to put down a monster */
#define MAXTRAPS	10

/** treas_room:
 * Add a treasure room */
static void
treas_room(void)
{
  struct room* room = &rooms[room_random()];
  int spots = (room->r_max.y - 2) * (room->r_max.x - 2) - MINTREAS;

  if (spots > (MAXTREAS - MINTREAS))
    spots = (MAXTREAS - MINTREAS);
  int num_monsters = os_rand_range(spots) + MINTREAS;

  for (int i = 0; i < num_monsters; ++i)
  {
    Coordinate monster_pos;
    Item* item = new_thing();

    Game::level->get_random_room_coord(room, &monster_pos, 2 * MAXTRIES, false);
    item->set_pos(monster_pos);
    Game::level->items.push_back(item);
    Game::level->set_ch(monster_pos, static_cast<char>(item->o_type));
  }

  /* fill up room with monsters from the next level down */
  int nm = os_rand_range(spots) + MINTREAS;
  if (nm < num_monsters + 2)
    nm = num_monsters + 2;

  spots = (room->r_max.y - 2) * (room->r_max.x - 2);
  if (nm > spots)
    nm = spots;
  Game::current_level++;
  while (nm--)
  {
    Coordinate monster_pos;
    spots = 0;
    if (Game::level->get_random_room_coord(room, &monster_pos, MAXTRIES, true))
    {
      Monster* monster = new Monster();
      monster_new(monster, monster_random(false), &monster_pos, room);
      monster->t_flags |= ISMEAN;	/* no sloughers in THIS room */
      monster_list.push_back(monster);
      monster_give_pack(monster);
      Game::level->set_monster(monster_pos, monster);
    }
  }
  Game::current_level--;
}

void
Level::create_loot()
{
  int i;

  /* Once you have found the amulet, the only way to get new stuff is
   * go down into the dungeon. */
  if (pack_contains_amulet() && Game::current_level < Game::max_level_visited)
      return;

  /* check for treasure rooms, and if so, put it in. */
  if (os_rand_range(TREAS_ROOM) == 0)
    treas_room();

  /* Do MAXOBJ attempts to put things on a level */
  for (i = 0; i < MAXOBJ; i++)
    if (os_rand_range(100) < 36)
    {
      /* Pick a new object and link it in the list */
      Item* obj = new_thing();
      this->items.push_back(obj);

      /* Put it somewhere */
      Coordinate pos;
      this->get_random_room_coord(nullptr, &pos, 0, false);
      obj->set_pos(pos);
      this->set_ch(obj->get_pos(), static_cast<char>(obj->o_type));
    }

  /* If he is really deep in the dungeon and he hasn't found the
   * amulet yet, put it somewhere on the ground */
  if (Game::current_level >= Game::amulet_min_level && !pack_contains_amulet())
  {
    Item* amulet = new_amulet();
    this->items.push_back(amulet);

    /* Put it somewhere */
    Coordinate pos;
    this->get_random_room_coord(nullptr, &pos, 0, false);
    amulet->set_pos(pos);
    this->set_ch(amulet->get_pos(), AMULET);
  }
}

Level::~Level() {

  /* Remove all monsters */
  for (Monster* mon : monster_list) {
    delete mon;
  }
  monster_list.clear();

  /* Remove all items */
  for (Item* item : this->items) {
    delete item;
  }
}

void
Level::create_traps() {
  if (os_rand_range(10) < Game::current_level) {
    int ntraps = os_rand_range(Game::current_level / 4) + 1;
    if (ntraps > MAXTRAPS) {
      ntraps = MAXTRAPS;
    }
    while (ntraps--) {
      /*
       * not only wouldn't it be NICE to have traps in mazes
       * (not that we care about being nice), since the trap
       * number is stored where the passage number is, we
       * can't actually do it.
       */
      do {
        this->get_random_room_coord(nullptr, &this->stairs_coord, 0, false);
      } while (this->get_ch(this->stairs_coord) != FLOOR);

      size_t trap_type = static_cast<size_t>(os_rand_range(NTRAPS));
      Game::level->set_not_real(this->stairs_coord);
      Game::level->set_trap_type(this->stairs_coord, trap_type);
    }
  }
}

void
Level::create_stairs() {
  this->get_random_room_coord(nullptr, &this->stairs_coord, 0, false);
  this->set_ch(this->stairs_coord, STAIRS);
}


Level::Level() {

  clear();
  this->places.resize(MAXLINES * MAXCOLS);
  this->passages.resize(12);

  this->create_rooms();
  this->create_passages();
  Game::levels_without_food++;      /* Levels with no food placed */
  this->create_loot();
  this->create_traps();
  this->create_stairs();

  /* Set room pointers for all monsters */
  for (Monster* mon : monster_list) {
    mon->t_room = this->get_room(mon->t_pos);
  }
}

place& Level::get_place(int x, int y) {
  size_t pos = static_cast<size_t>((x << 5) + y);
  return this->places.at(pos);
}

Monster*
Level::get_monster(int x, int y) {
  return this->get_place(x, y).p_monst;
}

Monster*
Level::get_monster(Coordinate const& coord) {
  return this->get_monster(coord.x, coord.y);
}

Item*
Level::get_item(int x, int y) {
  auto results = find_if(this->items.begin(), this->items.end(),
      [&] (Item* i) {
    return i->get_x() == x && i->get_y() == y;
  });

  return results == this->items.end()
    ? nullptr
    : *results;
}

Item*
Level::get_item(Coordinate const& coord) {
  return this->get_item(coord.x, coord.y);
}

void
Level::set_monster(int x, int y, Monster* monster) {
  this->get_place(x, y).p_monst = monster;
}

void
Level::set_monster(Coordinate const& coord, Monster* monster) {
  this->set_monster(coord.x, coord.y, monster);
}

bool
Level::is_passage(int x, int y) {
  return this->get_place(x, y).is_passage;
}

bool
Level::is_passage(Coordinate const& coord) {
  return this->is_passage(coord.x, coord.y);
}

bool
Level::is_discovered(int x, int y) {
  return this->get_place(x, y).is_discovered;
}

bool
Level::is_discovered(Coordinate const& coord) {
  return this->is_discovered(coord.x, coord.y);
}

bool
Level::is_real(int x, int y) {
  return this->get_place(x, y).is_real;
}

bool
Level::is_real(Coordinate const& coord) {
  return this->is_real(coord.x, coord.y);
}

void
Level::set_passage(int x, int y) {
  this->get_place(x, y).is_passage = true;
}

void
Level::set_passage(Coordinate const& coord) {
  this->set_passage(coord.x, coord.y);
}

void
Level::set_discovered(int x, int y) {
  this->get_place(x, y).is_discovered = true;
}

void
Level::set_discovered(Coordinate const& coord) {
  this->set_discovered(coord.x, coord.y);
}

void
Level::set_real(int x, int y) {
  this->get_place(x, y).is_real = true;
}

void
Level::set_real(Coordinate const& coord) {
  this->set_real(coord.x, coord.y);
}

void
Level::set_not_real(int x, int y) {
  this->get_place(x, y).is_real = false;
}

void
Level::set_not_real(Coordinate const& coord) {
  this->set_not_real(coord.x, coord.y);
}

char
Level::get_ch(int x, int y) {
  return this->get_place(x, y).p_ch;
}

char
Level::get_ch(Coordinate const& coord) {
  return this->get_ch(coord.x, coord.y);
}

void
Level::set_ch(int x, int y, char ch) {
  this->get_place(x, y).p_ch = ch;
}

void
Level::set_ch(Coordinate const& coord, char ch) {
  this->set_ch(coord.x, coord.y, ch);
}

void
Level::set_trap_type(int x, int y, size_t type) {
  this->get_place(x, y).trap_type = type;
}

void
Level::set_trap_type(Coordinate const& coord, size_t type) {
  this->set_trap_type(coord.x, coord.y, type);
}

void
Level::set_passage_number(int x, int y, size_t num) {
  this->get_place(x, y).passage_number = num;
}

void
Level::set_passage_number(Coordinate const& coord, size_t num) {
  this->set_passage_number(coord.x, coord.y, num);
}

char
Level::get_type(int x, int y)
{
  Monster* monster = this->get_monster(x, y);
  return monster == nullptr
    ? this->get_ch(x, y)
    : monster->t_disguise;
}

char
Level::get_type(Coordinate const& coord)
{
  return this->get_type(coord.x, coord.y);
}

size_t
Level::get_trap_type(int x, int y) {
  return this->get_place(x, y).trap_type;
}

size_t
Level::get_trap_type(Coordinate const& coord) {
  return this->get_trap_type(coord.x, coord.y);
}

size_t
Level::get_passage_number(int x, int y) {
  return this->get_place(x, y).passage_number;
}

size_t
Level::get_passage_number(Coordinate const& coord) {
  return this->get_passage_number(coord.x, coord.y);
}

room*
Level::get_room(Coordinate const& coord) {

  if (this->is_passage(coord)) {
    return this->get_passage(coord);
  }

  for (struct room* rp = rooms; rp < &rooms[ROOMS_MAX]; rp++) {
    if (coord.x <= rp->r_pos.x + rp->r_max.x
        && rp->r_pos.x <= coord.x
        && coord.y <= rp->r_pos.y + rp->r_max.y
        && rp->r_pos.y <= coord.y) {
      return rp;
    }
  }

  // Not sure if this should be an error, or just if monster is in corridor?
  //error("Coordinate was not in any room");
  return nullptr;
}

room* Level::get_passage(Coordinate const& coord) {
  return &this->passages.at(this->get_passage_number(coord));
}

Coordinate const& Level::get_stairs_pos() const {
  return this->stairs_coord;
}

int Level::get_stairs_x() const {
  return this->stairs_coord.x;
}

int Level::get_stairs_y() const {
  return this->stairs_coord.y;
}


