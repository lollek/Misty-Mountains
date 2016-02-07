#include <list>

#include "amulet.h"
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
#include "os.h"
#include "rogue.h"

#include "level.h"

using namespace std;

int constexpr Level::max_items;
int constexpr Level::max_monsters;
int constexpr Level::max_traps;
int constexpr Level::treasure_room_chance;
int constexpr Level::treasure_room_max_items;
int constexpr Level::treasure_room_min_items;

void Level::create_treasure_room() {

  room& room = *get_random_room();
  int spots = max((room.r_max.y - 2) * (room.r_max.x - 2) - treasure_room_min_items,
              treasure_room_max_items - treasure_room_min_items);

  int num_items = os_rand_range(spots) + treasure_room_min_items;
  for (int i = 0; i < num_items; ++i) {
    Coordinate item_pos;
    Item* item = Item::random();

    get_random_room_coord(&room, &item_pos, 2 * max_monsters, false);
    item->set_position(item_pos);
    items.push_back(item);
  }

  // fill up room with monsters from the next level down
  int num_monsters = max({os_rand_range(spots) + treasure_room_min_items,
                         num_items + 2,
                         (room.r_max.y - 2) * (room.r_max.x - 2)});

  Game::current_level++;
  for (int i = 0; i < num_monsters; ++i) {
    Coordinate monster_pos;
    if (get_random_room_coord(&room, &monster_pos, max_monsters, true)) {
      Monster* monster = new Monster(Monster::random_monster_type(), monster_pos, &room);
      monster->set_mean();  // no sloughers in THIS room
      monsters.push_back(monster);
      monster_give_pack(monster);
      set_monster(monster_pos, monster);
    }
  }
  Game::current_level--;
}

void Level::create_loot() {

  // Once you have found the amulet, the only way to get new stuff is
  // go down into the dungeon.
  if (pack_contains_amulet() && Game::current_level < Game::max_level_visited) {
      return;
  }

  // check for treasure rooms, and if so, put it in.
  if (os_rand_range(100) < treasure_room_chance) {
    create_treasure_room();
  }

  // Do some attempts to put things on a level
  for (int i = 0; i < max_items; i++) {
    if (os_rand_range(100) < 36) {
      // Pick a new object and link it in the list
      Item* obj = Item::random();
      items.push_back(obj);

      // Put it somewhere
      Coordinate pos;
      get_random_room_coord(nullptr, &pos, 0, false);
      obj->set_position(pos);
    }
  }

  // If he is really deep in the dungeon and he hasn't found the
  // amulet yet, put it somewhere on the ground
  if (Game::current_level >= Game::amulet_min_level && !pack_contains_amulet()) {
    Amulet* amulet = new Amulet();
    items.push_back(amulet);

    // Put it somewhere
    Coordinate pos;
    get_random_room_coord(nullptr, &pos, 0, false);
    amulet->set_position(pos);
  }
}

Level::~Level() {

  for (Monster* mon : monsters) {
    delete mon;
  }

  for (Item* item : items) {
    delete item;
  }
}

void Level::create_traps() {
  if (os_rand_range(10) < Game::current_level) {
    int ntraps = max(os_rand_range(Game::current_level / 4) + 1, max_traps);
    for (int i = 0; i < ntraps; ++i) {
      do {
        get_random_room_coord(nullptr, &stairs_coord, 0, false);
      } while (get_ch(stairs_coord) != FLOOR);

      size_t trap_type = static_cast<size_t>(os_rand_range(Trap::NTRAPS));
      set_not_real(stairs_coord);
      set_trap_type(stairs_coord, trap_type);
    }
  }
}

void Level::create_stairs() {
  get_random_room_coord(nullptr, &stairs_coord, 0, false);
  set_ch(stairs_coord, STAIRS);
}


Level::Level() {

  clear();
  rooms.resize(9);
  if (player != nullptr) {
    player->set_previous_room(nullptr);
  }
  tiles.resize(MAXLINES * MAXCOLS);
  passages.resize(12);

  create_rooms();
  create_passages();
  Game::levels_without_food++;
  create_loot();
  create_traps();
  create_stairs();

  // Set room pointers for all monsters
  for (Monster* mon : monsters) {
    mon->set_room(get_room(mon->get_position()));
  }
}

Tile& Level::get_tile(int x, int y) {
  size_t pos = static_cast<size_t>((x << 5) + y);
  return tiles.at(pos);
}

Monster* Level::get_monster(int x, int y) {
  return get_tile(x, y).p_monst;
}

Monster* Level::get_monster(Coordinate const& coord) {
  return get_monster(coord.x, coord.y);
}

Item* Level::get_item(int x, int y) {
  auto results = find_if(items.begin(), items.end(),
      [&] (Item* i) {
    return i->get_x() == x && i->get_y() == y;
  });

  return results == items.end()
    ? nullptr
    : *results;
}

Item* Level::get_item(Coordinate const& coord) {
  return get_item(coord.x, coord.y);
}

void Level::set_monster(int x, int y, Monster* monster) {
  get_tile(x, y).p_monst = monster;
}

void Level::set_monster(Coordinate const& coord, Monster* monster) {
  set_monster(coord.x, coord.y, monster);
}

bool Level::is_passage(int x, int y) {
  return get_tile(x, y).is_passage;
}

bool Level::is_passage(Coordinate const& coord) {
  return is_passage(coord.x, coord.y);
}

bool Level::is_discovered(int x, int y) {
  return get_tile(x, y).is_discovered;
}

bool Level::is_discovered(Coordinate const& coord) {
  return is_discovered(coord.x, coord.y);
}

bool Level::is_real(int x, int y) {
  return get_tile(x, y).is_real;
}

bool Level::is_real(Coordinate const& coord) {
  return is_real(coord.x, coord.y);
}

void Level::set_passage(int x, int y) {
  get_tile(x, y).is_passage = true;
}

void Level::set_passage(Coordinate const& coord) {
  set_passage(coord.x, coord.y);
}

void Level::set_discovered(int x, int y) {
  get_tile(x, y).is_discovered = true;
}

void Level::set_discovered(Coordinate const& coord) {
  set_discovered(coord.x, coord.y);
}

void Level::set_real(int x, int y) {
  get_tile(x, y).is_real = true;
}

void Level::set_real(Coordinate const& coord) {
  set_real(coord.x, coord.y);
}

void Level::set_not_real(int x, int y) {
  get_tile(x, y).is_real = false;
}

void Level::set_not_real(Coordinate const& coord) {
  set_not_real(coord.x, coord.y);
}

char Level::get_ch(int x, int y) {
  return get_tile(x, y).p_ch;
}

char Level::get_ch(Coordinate const& coord) {
  return get_ch(coord.x, coord.y);
}

void Level::set_ch(int x, int y, char ch) {
  switch (ch) {
    case HWALL: case VWALL: case SHADOW: case PASSAGE: case STAIRS: case DOOR:
    case FLOOR: case TRAP:
      get_tile(x, y).p_ch = ch;
      return;

    default:
      error("Cannot set level character to " + string(1, ch));
  }
}

void Level::set_ch(Coordinate const& coord, char ch) {
  set_ch(coord.x, coord.y, ch);
}

void Level::set_trap_type(int x, int y, size_t type) {
  get_tile(x, y).trap_type = type;
}

void Level::set_trap_type(Coordinate const& coord, size_t type) {
  set_trap_type(coord.x, coord.y, type);
}

void Level::set_passage_number(int x, int y, size_t num) {
  get_tile(x, y).passage_number = num;
}

void Level::set_passage_number(Coordinate const& coord, size_t num) {
  set_passage_number(coord.x, coord.y, num);
}

char Level::get_type(int x, int y) {
  Monster* monster = get_monster(x, y);
  return monster == nullptr
    ? get_ch(x, y)
    : monster->t_disguise;
}

char Level::get_type(Coordinate const& coord) {
  return get_type(coord.x, coord.y);
}

size_t Level::get_trap_type(int x, int y) {
  return get_tile(x, y).trap_type;
}

size_t Level::get_trap_type(Coordinate const& coord) {
  return get_trap_type(coord.x, coord.y);
}

size_t Level::get_passage_number(int x, int y) {
  return get_tile(x, y).passage_number;
}

size_t Level::get_passage_number(Coordinate const& coord) {
  return get_passage_number(coord.x, coord.y);
}

room* Level::get_room(Coordinate const& coord) {

  if (is_passage(coord)) {
    return get_passage(coord);
  }

  for (struct room& room : rooms) {
    if (coord.x <= room.r_pos.x + room.r_max.x
        && room.r_pos.x <= coord.x
        && coord.y <= room.r_pos.y + room.r_max.y
        && room.r_pos.y <= coord.y) {
      return &room;
    }
  }

  // Not sure if this should be an error, or just if monster is in corridor?
  //error("Coordinate was not in any room");
  return nullptr;
}

room* Level::get_passage(Coordinate const& coord) {
  return &passages.at(get_passage_number(coord));
}

Coordinate const& Level::get_stairs_pos() const {
  return stairs_coord;
}

int Level::get_stairs_x() const {
  return stairs_coord.x;
}

int Level::get_stairs_y() const {
  return stairs_coord.y;
}

