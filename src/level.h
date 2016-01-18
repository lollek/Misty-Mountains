#pragma once

#include <list>
#include <vector>
#include <string>

#include "monster.h"
#include "item.h"
#include "io.h"

/* Flags for level map */
#define F_PASS		0x80		/* is a passageway */
#define F_SEEN		0x40		/* have seen this spot before */
#define F_DROPPED	0x20		/* object was dropped here */
#define F_LOCKED	0x20		/* door is locked */
#define F_REAL		0x10		/* what you see is what you get */
#define F_PNUM		0x0f		/* passage number mask */
#define F_TMASK		0x07		/* trap number mask */

/* describe a place on the level map */
struct place {
  place() : p_ch(SHADOW), p_flags(F_REAL), p_monst(nullptr) {}

  char     p_ch;
  char     p_flags;
  Monster* p_monst;
};

class Level {
public:
  Level();
  ~Level();

  // Getters
  Monster* get_monster(int x, int y);
  Monster* get_monster(Coordinate const& coord);
  Item* get_item(int x, int y);
  Item* get_item(Coordinate const& coord);
  char get_flags(int x, int y);
  char get_flags(Coordinate const& coord);
  bool get_flag_seen(int x, int y);
  bool get_flag_seen(Coordinate const& coord);
  bool get_flag_passage(int x, int y);
  bool get_flag_passage(Coordinate const& coord);
  bool get_flag_real(int x, int y);
  bool get_flag_real(Coordinate const& coord);
  char get_ch(int x, int y);
  char get_ch(Coordinate const& coord);
  char get_trap_type(int x, int y);
  char get_trap_type(Coordinate const& coord);
  char get_type(int x, int y);
  char get_type(Coordinate const& coord);
  bool get_random_room_coord(room* room, Coordinate* coord, int tries, bool monster);
  room* get_room(Coordinate const& coord);
  room* get_passage(Coordinate const& coord);
  Coordinate const& get_stairs_pos() const;
  int get_stairs_x() const;
  int get_stairs_y() const;

  // Setters
  void set_monster(int x, int y, Monster* monster);
  void set_monster(Coordinate const& coord, Monster* monster);
  void set_flags(int x, int y, char flags);
  void set_flags(Coordinate const& coord, char flags);
  void set_flag_seen(int x, int y);
  void set_flag_seen(Coordinate const& coord);
  void set_flag_passage(int x, int y);
  void set_flag_passage(Coordinate const& coord);
  void set_flag_real(int x, int y);
  void set_flag_real(Coordinate const& coord);
  void set_flag_notreal(int x, int y);
  void set_flag_notreal(Coordinate const& coord);
  void set_ch(int x, int y, char ch);
  void set_ch(Coordinate const& coord, char ch);

  // Misc
  void wizard_show_passages();

  // Variables
  std::list<Item*> items;   // List of items on level

private:

  // Parts of constructor
  void create_rooms();
  void create_passages();
  void create_loot();
  void create_traps();
  void create_stairs();

  // Part of create_rooms()
  void draw_room(room const& room);
  void draw_maze(room const& room);
  void draw_maze_recursive(int y, int x, int starty, int startx, int maxy, int maxx);

  // Part of create_passages()
  void place_door(room* room, Coordinate* coord);
  void place_passage(Coordinate* coord);
  void connect_passages(int r1, int r2);
  void number_passage(int x, int y);

  // Misc
  place& get_place(int x, int y);

  // Variables
  std::vector<place> places;        // level map
  std::vector<room>  passages;      // Passages between rooms
  Coordinate         stairs_coord;  // Where the stairs are
};
