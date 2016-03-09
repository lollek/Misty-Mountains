#include <list>
#include <algorithm>

#include "item/amulet.h"
#include "error_handling.h"
#include "game.h"
#include "traps.h"
#include "io.h"
#include "daemons.h"
#include "monster.h"
#include "misc.h"
#include "player.h"
#include "level/rooms.h"
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
  int spots = min((room.r_max.y - 2) * (room.r_max.x - 2) - treasure_room_min_items,
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
  int num_monsters = min({os_rand_range(spots) + treasure_room_min_items,
                         num_items + 2,
                         (room.r_max.y - 2) * (room.r_max.x - 2)});

  Game::current_level++;
  for (int i = 0; i < num_monsters; ++i) {
    Coordinate monster_pos;
    if (get_random_room_coord(&room, &monster_pos, max_monsters, true)) {
      Monster::Type mon_type = Monster::random_monster_type_for_level();
      Monster* monster = new Monster(mon_type, monster_pos);
      monster->set_mean();  // no sloughers in THIS room
      monsters.push_back(monster);
      set_monster(monster_pos, monster);
    }
  }
  Game::current_level--;
}

void Level::create_loot() {

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
  if (player != nullptr && !player->pack_contains_amulet() &&
      Game::current_level >= Game::amulet_min_level) {
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

  if (shop != nullptr) {
    delete shop;
  }
}

void Level::create_traps() {
  if (os_rand_range(10) < Game::current_level) {
    int ntraps = min(os_rand_range(Game::current_level / 4) + 1, max_traps);
    Coordinate coord;
    for (int i = 0; i < ntraps; ++i) {
      do {
        get_random_room_coord(nullptr, &coord, 0, false);
      } while (get_tile(coord) != Tile::Floor);

      set_not_real(coord);
      Trap::Type trap_type = static_cast<Trap::Type>(os_rand_range(Trap::NTRAPS));
      set_trap_type(coord, trap_type);
    }
  }
}

void Level::create_stairs() {
  Coordinate stairs_coord;
  get_random_room_coord(nullptr, &stairs_coord, 0, false);
  set_tile(stairs_coord, Tile::StairsUp);

  get_random_room_coord(nullptr, &stairs_coord, 0, false);
  set_tile(stairs_coord, Tile::StairsDown);
}

void Level::create_shop() {
  Coordinate shop_coord;
  get_random_room_coord(nullptr, &shop_coord, 0, false);
  set_tile(shop_coord, Tile::Shop);
}


Level::Level() : items(), monsters(), shop(), rooms(), tiles() {
  tiles.resize(IO::map_width * IO::map_height);
  if (player != nullptr) {
    player->set_previous_room(nullptr);
  }

  Game::io->clear_screen();

  rooms.resize(9);
  create_rooms();

  create_passages();

  Game::levels_without_food++;
  create_loot();
  create_traps();
  create_stairs();

  if (Game::current_level == 1) {
    create_shop();
  }
}

Tile& Level::tile(int x, int y) {
  size_t pos = static_cast<size_t>((x << 5) + y);
  return tiles.at(pos);
}

Monster* Level::get_monster(int x, int y) {
  return tile(x, y).monster;
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
  tile(x, y).monster = monster;
}

void Level::set_monster(Coordinate const& coord, Monster* monster) {
  set_monster(coord.x, coord.y, monster);
}

bool Level::is_passage(int x, int y) {
  return tile(x, y).is_passage;
}

bool Level::is_passage(Coordinate const& coord) {
  return is_passage(coord.x, coord.y);
}

bool Level::is_discovered(int x, int y) {
  return tile(x, y).is_discovered;
}

bool Level::is_discovered(Coordinate const& coord) {
  return is_discovered(coord.x, coord.y);
}

bool Level::is_real(int x, int y) {
  return tile(x, y).is_real;
}

bool Level::is_real(Coordinate const& coord) {
  return is_real(coord.x, coord.y);
}

void Level::set_passage(int x, int y) {
  tile(x, y).is_passage = true;
}

void Level::set_passage(Coordinate const& coord) {
  set_passage(coord.x, coord.y);
}

void Level::set_discovered(int x, int y) {
  tile(x, y).is_discovered = true;
}

void Level::set_discovered(Coordinate const& coord) {
  set_discovered(coord.x, coord.y);
}

void Level::set_real(int x, int y) {
  tile(x, y).is_real = true;
}

void Level::set_real(Coordinate const& coord) {
  set_real(coord.x, coord.y);
}

void Level::set_not_real(int x, int y) {
  tile(x, y).is_real = false;
}

void Level::set_not_real(Coordinate const& coord) {
  set_not_real(coord.x, coord.y);
}

Tile::Type Level::get_tile(int x, int y) {
  return tile(x, y).type;
}

Tile::Type Level::get_tile(Coordinate const& coord) {
  return get_tile(coord.x, coord.y);
}

void Level::set_tile(int x, int y, Tile::Type type) {
  tile(x, y).type = type;
}

void Level::set_tile(Coordinate const& coord, Tile::Type tile) {
  set_tile(coord.x, coord.y, tile);
}

void Level::set_trap_type(int x, int y, Trap::Type type) {
  tile(x, y).trap_type = type;
}

void Level::set_trap_type(Coordinate const& coord, Trap::Type type) {
  set_trap_type(coord.x, coord.y, type);
}

Trap::Type Level::get_trap_type(int x, int y) {
  return tile(x, y).trap_type;
}

Trap::Type Level::get_trap_type(Coordinate const& coord) {
  return get_trap_type(coord.x, coord.y);
}

room* Level::get_room(Coordinate const& coord) {
  for (struct room& room : rooms) {
    if (coord.x <= room.r_pos.x + room.r_max.x
        && room.r_pos.x <= coord.x
        && coord.y <= room.r_pos.y + room.r_max.y
        && room.r_pos.y <= coord.y) {
      return &room;
    }
  }

  // Was corridor, probably
  return nullptr;
}

bool Level::can_step(int x, int y) {
  switch(get_tile(x, y)) {
    case Tile::Wall: case Tile::ClosedDoor: case Tile::Shop:
      return false;

    case Tile::Floor: case Tile::OpenDoor: case Tile::StairsDown:
    case Tile::Trap:  case Tile::StairsUp:
      break;
  }

  return get_monster(x, y) == nullptr;
}

bool Level::can_step(Coordinate const& coord) {
  return can_step(coord.x, coord.y);
}

bool Level::is_dark(int x, int y) {
  return tile(x, y).is_dark;
}

bool Level::is_dark(Coordinate const& coord) {
  return is_dark(coord.x, coord.y);
}

void Level::discover_map() {
  for (int y = 0; y < IO::map_height; y++) {
    for (int x = 0; x < IO::map_width; x++) {
      Game::level->set_discovered(x, y);

      // Unhide any features
      if (!Game::level->is_real(x, y)) {
        switch (Game::level->get_tile(x, y)) {

          // Most things are always what they seem
          case ::Tile::OpenDoor: case ::Tile::ClosedDoor: case ::Tile::StairsDown:
          case ::Tile::StairsUp: case ::Tile::Shop: case ::Tile::Trap:
            error("Unexpected tiletype was not real");

          // Check if walls are actually hidden doors
          case ::Tile::Wall: {
            Game::level->set_tile(x, y, ::Tile::ClosedDoor);
            Game::level->set_real(x, y);
          } break;

          // Floor can be traps.
          case ::Tile::Floor: {
            Game::level->set_tile(x, y, ::Tile::Trap);
            Game::level->set_real(x, y);
          } break;
        }
      }
    }
  }
}


