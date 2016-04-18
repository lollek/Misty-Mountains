#include <exception>

using namespace std;

#include "coordinate.h"
#include "error_handling.h"
#include "game.h"
#include "io.h"
#include "item/gold.h"
#include "level.h"
#include "misc.h"
#include "monster.h"
#include "os.h"
#include "player.h"
#include "rogue.h"

#include "level/rooms.h"

/* position matrix for maze positions */
struct spot {
  int nexits;
  Coordinate exits[4];
  int used;
};

static spot maze[IO::map_height / 3 + 1][IO::map_width / 3 + 1];

/* Account for maze exits */
static void room_accnt_maze(int y, int x, int ny, int nx) {
  spot* sp{&maze[y][x]};
  Coordinate* cp;

  for (cp = sp->exits; cp < &sp->exits[sp->nexits]; cp++) {
    if (cp->y == ny && cp->x == nx) { return; }
  }

  cp->y = ny;
  cp->x = nx;
}

/* Dig out from around where we are now, if possible */
void Level::draw_maze_recursive(int y, int x, int starty, int startx, int maxy,
                                int maxx) {
  int nexty{0};
  int nextx{0};

  for (;;) {
    Coordinate const del[4] = {{2, 0}, {-2, 0}, {0, 2}, {0, -2}};
    int cnt{0};
    for (unsigned i{0}; i < sizeof(del) / sizeof(*del); ++i) {
      int const newy{y + del[i].y};
      int const newx{x + del[i].x};

      if (newy < 0 || newy > maxy || newx < 0 || newx > maxx ||
          is_passage(newx + startx, newy + starty)) {
        continue;
      }

      if (os_rand_range(++cnt) == 0) {
        nexty = newy;
        nextx = newx;
      }
    }

    if (cnt == 0) { return; }

    room_accnt_maze(y, x, nexty, nextx);
    room_accnt_maze(nexty, nextx, y, x);

    Coordinate pos;
    if (nexty == y) {
      pos.y = y + starty;
      if (nextx - x < 0) {
        pos.x = nextx + startx + 1;
      } else {
        pos.x = nextx + startx - 1;
      }
    } else {
      pos.x = x + startx;
      if (nexty - y < 0) {
        pos.y = nexty + starty + 1;
      } else {
        pos.y = nexty + starty - 1;
      }
    }
    place_passage(&pos);

    pos.y = nexty + starty;
    pos.x = nextx + startx;
    place_passage(&pos);

    draw_maze_recursive(nexty, nextx, starty, startx, maxy, maxx);
  }
}

/* Dig a maze */
void Level::draw_maze(room const& rp) {
  for (spot* sp{&maze[0][0]};
       sp <= &maze[IO::map_height / 3][IO::map_width / 3]; sp++) {
    sp->used = false;
    sp->nexits = 0;
  }

  int const y{(os_rand_range(rp.r_max.y) / 2) * 2};
  int const x{(os_rand_range(rp.r_max.x) / 2) * 2};

  // TODO: Is this a typo/incorrect? maybe x + rp->r_pos.x
  Coordinate pos(y + rp.r_pos.x, y + rp.r_pos.y);
  place_passage(&pos);

  draw_maze_recursive(y, x, rp.r_pos.y, rp.r_pos.x, rp.r_max.y, rp.r_max.x);
}

/* Draw a box around a room and lay down the floor for normal
 * rooms; for maze rooms, draw maze. */
void Level::draw_room(room const& rp) {
  /* Draw left + right side */
  for (int y{rp.r_pos.y + 1}; y <= rp.r_max.y + rp.r_pos.y - 1; y++) {
    set_tile(rp.r_pos.x, y, Tile::Wall);
    set_tile(rp.r_pos.x + rp.r_max.x - 1, y, Tile::Wall);
  }

  /* Draw top + bottom side */
  for (int x{rp.r_pos.x}; x <= rp.r_pos.x + rp.r_max.x - 1; x++) {
    set_tile(x, rp.r_pos.y, Tile::Wall);
    set_tile(x, rp.r_pos.y + rp.r_max.y - 1, Tile::Wall);
  }

  /* Put the floor down */
  for (int y{rp.r_pos.y + 1}; y < rp.r_pos.y + rp.r_max.y - 1; y++) {
    for (int x{rp.r_pos.x + 1}; x < rp.r_pos.x + rp.r_max.x - 1; x++) {
      Tile& t{tile(x, y)};
      t.type = Tile::Floor;
      t.is_dark = rp.is_dark;
    }
  }
}

static void room_place_gone_room(Coordinate const* max_size,
                                 Coordinate const* top, room* room) {
  /** Place a gone room.  Make certain that there is a blank line
   * for passage drawing.  */
  do {
    room->r_pos.x = top->x + os_rand_range(max_size->x - 3) + 1;
    room->r_pos.y = top->y + os_rand_range(max_size->y - 2) + 1;
    room->r_max.x = -IO::map_width;
    room->r_max.y = -IO::map_height;
  } while (!(room->r_pos.y > 0 && room->r_pos.y < IO::map_height - 1 &&
             room->r_pos.x > 0 && room->r_pos.x < IO::map_width - 1));
}

void Level::create_rooms() {
  /* Put the gone rooms, if any, on the level */
  int left_out{os_rand_range(4)};
  for (int i{0}; i < left_out; i++) { get_random_room()->is_gone = true; }

  /* dig and populate all the rooms on the level */
  if (rooms.size() != 9) {
    error(
        "This functions expects there to be exacly 9 rooms"
        " but currently there are " +
        to_string(rooms.size()));
  }

  /* maximum room size */
  Coordinate const bsze(IO::map_width / 3, IO::map_height / 3);

  for (int i{0}; i < static_cast<int>(rooms.size()); i++) {
    room& room{rooms.at(static_cast<size_t>(i))};

    /* Find upper left corner of box that this room goes in */
    Coordinate const top((i % 3) * bsze.x + 1, (i / 3) * bsze.y);

    if (room.is_gone) {
      room_place_gone_room(&bsze, &top, &room);
      continue;
    }

    /* set room type */
    if (os_rand_range(10) < Game::current_level - 1) {
      room.is_dark = true;
      if (os_rand_range(15) == 0) { room.is_maze = true; }
    }

    /* Find a place and size for a random room */
    if (room.is_maze) {
      room.r_max.x = bsze.x - 1;
      room.r_max.y = bsze.y - 1;
      room.r_pos.x = top.x;
      room.r_pos.y = top.y;
      if (room.r_pos.y == 0) {
        room.r_pos.y++;
        room.r_max.y--;
      } else if (room.r_pos.x == 0) {
        room.r_pos.x++;
        room.r_max.x--;
      }

    } else {
      do {
        room.r_max.x = os_rand_range(bsze.x - 4) + 4;
        room.r_max.y = os_rand_range(bsze.y - 4) + 4;
        room.r_pos.x = top.x + os_rand_range(bsze.x - room.r_max.x);
        room.r_pos.y = top.y + os_rand_range(bsze.y - room.r_max.y);
      } while (room.r_pos.y == 0 || room.r_pos.x == 0);
    }

    if (room.is_maze) {
      draw_maze(room);
    } else {
      draw_room(room);
    }

    /* Put the monster in */
    if (os_rand_range(100) < 25) {
      Coordinate mp;
      get_random_room_coord(&room, &mp, 0, true);
      Monster::Type const mon_type{Monster::random_monster_type_for_level()};
      Monster* monster{new Monster(mon_type, mp)};
      monsters.push_back(monster);
      set_monster(mp, monster);
    }
  }
}

bool Level::get_random_room_coord(room* room, Coordinate* coord, int tries,
                                  bool monst) {
  bool const limited_tries{tries > 0};
  char const compchar{Tile::Floor};
  bool const pickroom{room == nullptr};

  for (;;) {
    if (limited_tries && tries-- == 0) { return false; }

    if (pickroom) { room = get_random_room(); }

    /* Pick a random position */
    coord->x = room->r_pos.x + os_rand_range(room->r_max.x - 2) + 1;
    coord->y = room->r_pos.y + os_rand_range(room->r_max.y - 2) + 1;

    Tile::Type ch{get_tile(*coord)};
    if (monst) {
      if (get_monster(*coord) == nullptr && can_step(*coord)) { return true; }
    } else if (ch == compchar) {
      return true;
    }
  }
}

room* Level::get_random_room() {
  for (;;) {
    room* room{&rooms.at(static_cast<size_t>(os_rand_range(rooms.size())))};
    if (room->is_gone) { continue; }

    return room;
  }
}
