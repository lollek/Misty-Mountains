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

Coordinate     level_stairs;
list<Item*>    level_items;

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
    level_items.push_back(item);
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
      level_items.push_back(obj);

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
    level_items.push_back(amulet);

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
  for (Item* item : level_items) {
    delete item;
  }
  level_items.clear();
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
        this->get_random_room_coord(nullptr, &level_stairs, 0, false);
      } while (this->get_ch(level_stairs) != FLOOR);

      char trapflag = this->get_flags(level_stairs);
      trapflag &= ~F_REAL;
      trapflag |= os_rand_range(NTRAPS);
      this->set_flags(level_stairs, trapflag);
    }
  }
}

void
Level::create_stairs() {
  this->get_random_room_coord(nullptr, &level_stairs, 0, false);
  this->set_ch(level_stairs, STAIRS);
}


Level::Level() {

  clear();

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

PLACE*
Level::get_place(int x, int y) {
  return &this->level_places[((x) << 5) + (y)];
}

PLACE*
Level::get_place(Coordinate const& coord) {
  return &this->level_places[((coord.x) << 5) + (coord.y)];
}

Monster*
Level::get_monster(int x, int y) {
  return this->level_places[(x << 5) + y].p_monst;
}

Monster*
Level::get_monster(Coordinate const& coord) {
  return this->level_places[(coord.x << 5) + coord.y].p_monst;
}

void
Level::set_monster(int x, int y, Monster* monster) {
  this->level_places[(x << 5) + y].p_monst = monster;
}

void
Level::set_monster(Coordinate const& coord, Monster* monster) {
  this->level_places[(coord.x << 5) + coord.y].p_monst = monster;
}

char
Level::get_flags(int x, int y) {
  return this->level_places[(x << 5) + y].p_flags;
}

char
Level::get_flags(Coordinate const& coord) {
  return this->level_places[(coord.x << 5) + coord.y].p_flags;
}

void
Level::set_flags(int x, int y, char flags) {
  this->level_places[(x << 5) + y].p_flags = flags;
}

void
Level::set_flags(Coordinate const& coord, char flags) {
  this->level_places[(coord.x << 5) + coord.y].p_flags = flags;
}

char
Level::get_ch(int x, int y) {
  return this->level_places[(x << 5) + y].p_ch;
}

char
Level::get_ch(Coordinate const& coord) {
  return this->level_places[(coord.x << 5) + coord.y].p_ch;
}

void
Level::set_ch(int x, int y, char ch) {
  this->level_places[(x << 5) + y].p_ch = ch;
}

void
Level::set_ch(Coordinate const& coord, char ch) {
  this->level_places[(coord.x << 5) + coord.y].p_ch = ch;
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
  Monster* monster = this->get_monster(coord);
  return monster == nullptr
    ? this->get_ch(coord)
    : monster->t_disguise;
}

room*
Level::get_room(Coordinate const& coord) {

  char fp = this->get_flags(coord);
  if (fp & F_PASS) {
    return &passages[fp & F_PNUM];
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


