#pragma once

#include <list>
#include <vector>
#include <string>

#include "level_rooms.h"
#include "traps.h"
#include "monster.h"
#include "item.h"
#include "tiles.h"
#include "shop.h"

class Level {
public:
  Level();
  ~Level();

  // Getters
  Monster* get_monster(int x, int y);
  Monster* get_monster(Coordinate const& coord);
  Item* get_item(int x, int y);
  Item* get_item(Coordinate const& coord);
  bool is_passage(int x, int y);
  bool is_passage(Coordinate const& coord);
  bool is_discovered(int x, int y);
  bool is_discovered(Coordinate const& coord);
  bool is_real(int x, int y);
  bool is_real(Coordinate const& coord);
  bool is_dark(int x, int y);
  bool is_dark(Coordinate const& coord);
  Tile::Type get_tile(int x, int y);
  Tile::Type get_tile(Coordinate const& coord);
  Trap::Type get_trap_type(int x, int y);
  Trap::Type get_trap_type(Coordinate const& coord);
  bool get_random_room_coord(room* room, Coordinate* coord, int tries, bool monster);
  room* get_room(Coordinate const& coord);
  room* get_random_room();

  // Setters
  void set_monster(int x, int y, Monster* monster);
  void set_monster(Coordinate const& coord, Monster* monster);
  void set_passage(int x, int y);
  void set_passage(Coordinate const& coord);
  void set_discovered(int x, int y);
  void set_discovered(Coordinate const& coord);
  void set_real(int x, int y);
  void set_real(Coordinate const& coord);
  void set_not_real(int x, int y);
  void set_not_real(Coordinate const& coord);
  void set_tile(int x, int y, Tile::Type tile);
  void set_tile(Coordinate const& coord, Tile::Type tile);
  void set_trap_type(int x, int y, Trap::Type type);
  void set_trap_type(Coordinate const& coord, Trap::Type type);

  // Misc
  bool can_step(int x, int y);
  bool can_step(Coordinate const& coord);

  // Variables
  std::list<Item*>    items;    // List of items on level
  std::list<Monster*> monsters; // List of monsters on level
  Shop*               shop;     // Ye local shop

private:

  // Parts of constructor
  static int constexpr max_items = 9;
  static int constexpr max_monsters = 10;
  static int constexpr max_traps = 10;
  static int constexpr treasure_room_chance = 5;
  static int constexpr treasure_room_max_items = 10;
  static int constexpr treasure_room_min_items = 2;

  void create_rooms();
  void create_passages();
  void create_loot();
  void create_traps();
  void create_stairs();
  void create_shop();

  // Part of create_rooms()
  void create_treasure_room();
  void draw_room(room const& room);
  void draw_maze(room const& room);
  void draw_maze_recursive(int y, int x, int starty, int startx, int maxy, int maxx);

  // Part of create_passages()
  void place_door(room* room, Coordinate* coord);
  void place_passage(Coordinate* coord);
  void connect_passages(int r1, int r2);
  void number_passage(int x, int y, bool new_passage_number);

  // Misc
  Tile& tile(int x, int y);

  // Variables
  std::vector<room>  rooms;         // all rooms on level
  std::vector<Tile>  tiles;        // level map
};
