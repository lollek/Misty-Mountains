#include <cstdlib>
#include <exception>

using namespace std;

#include "error_handling.h"
#include "game.h"
#include "coordinate.h"
#include "io.h"
#include "os.h"
#include "level_rooms.h"
#include "rogue.h"

#include "level.h"

static int pnum;
static bool newpnum;

/** numpass:
 * Number a passageway square and its brethren */
void
Level::number_passage(int x, int y)
{
  if (x >= NUMCOLS || x < 0 || y >= NUMLINES || y <= 0) {
    return;
  }

  if (get_passage_number(x, y) != 0) {
    return;
  }

  if (newpnum) {
    pnum++;
    newpnum = false;
  }

  /* check to see if it is a door or secret door, i.e., a new exit,
   * or a numerable type of place */
  char ch = get_ch(x, y);
  if (ch == DOOR || (!is_real(x, y) && (ch == VWALL || ch == HWALL))) {
    struct room& rp = passages.at(static_cast<size_t>(pnum));
    rp.r_exit[rp.r_nexits].y = y;
    rp.r_exit[rp.r_nexits++].x = x;
  }

  else if (!is_passage(x, y)) {
    return;
  }

  set_passage_number(x, y, static_cast<size_t>(pnum));

  /* recurse on the surrounding places */
  number_passage(x, y + 1);
  number_passage(x, y - 1);
  number_passage(x + 1, y);
  number_passage(x - 1, y);
}

/** door:
 * Add a door or possibly a secret door.  Also enters the door in
 * the exits array of the room.  */
void
Level::place_door(room* room, Coordinate* coord) {

  room->r_exit[room->r_nexits++] = *coord;
  if (room->r_flags & ISMAZE) {
    return;
  }

  if (os_rand_range(10) + 1 < Game::current_level && os_rand_range(5) == 0) {
    if (coord->y == room->r_pos.y || coord->y == room->r_pos.y + room->r_max.y - 1) {
      set_ch(*coord, HWALL);
    } else {
      set_ch(*coord, VWALL);
    }
    set_not_real(*coord);
  } else {
    set_ch(*coord, DOOR);
  }
}


/** conn:
 * Draw a corridor from a room in a certain direction. */
void
Level::connect_passages(int r1, int r2) {
  int rm;
  char direc;
  if (r1 < r2) {
    rm = r1;
    if (r1 + 1 == r2) {
      direc = 'r';
    } else {
      direc = 'd';
    }

  } else {
    rm = r2;
    if (r2 + 1 == r1) {
      direc = 'r';
    } else {
      direc = 'd';
    }
  }

  room* room_from = &rooms[rm];
  room* room_to = nullptr;           /* room pointer of dest */
  Coordinate start_pos;                  /* start of move */
  Coordinate end_pos;                    /* end of move */
  Coordinate turn_delta;                 /* direction to turn */
  Coordinate del;                        /* direction of move */
  int distance = 0;                      /* distance to move */
  int turn_distance = 0;                 /* how far to turn */

    /* Set up the movement variables, in two cases:
     * first drawing one down.  */
  if (direc == 'd') {
    room_to = &rooms[rm + 3];
    del.x = 0;
    del.y = 1;
    start_pos.x = room_from->r_pos.x;
    start_pos.y = room_from->r_pos.y;
    end_pos.x = room_to->r_pos.x;
    end_pos.y = room_to->r_pos.y;

    /* if not gone pick door pos */
    if (!(room_from->r_flags & ISGONE)) {
      do {
        start_pos.x = room_from->r_pos.x + os_rand_range(room_from->r_max.x - 2)
          + 1;
        start_pos.y = room_from->r_pos.y + room_from->r_max.y - 1;
      } while ((room_from->r_flags & ISMAZE)
             && !is_passage(start_pos));
    }

    if (!(room_to->r_flags & ISGONE)) {
      do {
        end_pos.x = room_to->r_pos.x + os_rand_range(room_to->r_max.x - 2) + 1;
      } while ((room_to->r_flags & ISMAZE)
             && !is_passage(end_pos));
    }

    distance = abs(start_pos.y - end_pos.y) - 1;
    turn_delta.y = 0;
    turn_delta.x = (start_pos.x < end_pos.x ? 1 : -1);

    turn_distance = abs(start_pos.x - end_pos.x);

  /* setup for moving right */
  } else if (direc == 'r') {

    room_to = &rooms[rm + 1];
    del.x = 1;
    del.y = 0;
    start_pos.x = room_from->r_pos.x;
    start_pos.y = room_from->r_pos.y;
    end_pos.x = room_to->r_pos.x;
    end_pos.y = room_to->r_pos.y;
    if (!(room_from->r_flags & ISGONE)) {
      do {
        start_pos.x = room_from->r_pos.x + room_from->r_max.x - 1;
        start_pos.y = room_from->r_pos.y + os_rand_range(room_from->r_max.y - 2)
          + 1;
      } while ((room_from->r_flags & ISMAZE)
               && !is_passage(start_pos));
    }

    if (!(room_to->r_flags & ISGONE)) {
      do {
        end_pos.y = room_to->r_pos.y + os_rand_range(room_to->r_max.y - 2) + 1;
      } while ((room_to->r_flags & ISMAZE)
             && !is_passage(end_pos));
    }

    distance = abs(start_pos.x - end_pos.x) - 1;
    turn_delta.y = (start_pos.y < end_pos.y ? 1 : -1);
    turn_delta.x = 0;
    turn_distance = abs(start_pos.y - end_pos.y);
  }

  else {
    io_msg("DEBUG: error in connection tables");
  }

  /* where turn starts */
  int turn_spot = os_rand_range(distance - 1) + 1;

  /* Draw in the doors on either side of the passage or just put #'s
   * if the rooms are gone.  */
  if (!(room_from->r_flags & ISGONE)) {
    place_door(room_from, &start_pos);
  } else {
    place_passage(&start_pos);
  }

  if (!(room_to->r_flags & ISGONE)) {
    place_door(room_to, &end_pos);
  } else {
    place_passage(&end_pos);
  }

  /* Get ready to move...  */
  Coordinate curr(start_pos.x, start_pos.y);
  while (distance > 0) {
    /* Move to new position */
    curr.x += del.x;
    curr.y += del.y;

    /* Check if we are at the turn place, if so do the turn */
    if (distance == turn_spot) {
      while (turn_distance--) {
        place_passage(&curr);
        curr.x += turn_delta.x;
        curr.y += turn_delta.y;
      }
    }

    /* Continue digging along */
    place_passage(&curr);
    distance--;
  }

  curr.x += del.x;
  curr.y += del.y;

  if (curr != end_pos) {
    error("Connectivity problem");
  }
}


void
Level::create_passages()
{
  struct rdes {
    bool conn[ROOMS_MAX];  /* possible to connect to room i? */
    bool isconn[ROOMS_MAX];/* connection been made to room i? */
    bool ingraph;         /* this room in graph already? */
  } rdes[ROOMS_MAX] = {
    { { 0, 1, 0, 1, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 1, 0, 1, 0, 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 0, 1, 0, 0, 0, 1, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 1, 0, 0, 0, 1, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 0, 1, 0, 1, 0, 1, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 0, 0, 1, 0, 1, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 0, 0, 0, 1, 0, 0, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 0, 0, 0, 0, 1, 0, 1, 0, 1 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 0, 0, 0, 0, 0, 1, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
  };

  for (room& passage : passages) {
  //            r_pos   r_max   r_gold r_goldval r_flags        r_nexits r_exit[12]
    passage = { {0, 0}, {0, 0}, {0, 0}, 0,       ISGONE|ISDARK, 0,       {{0,0}} };
  }

  /* reinitialize room graph description */
  for (struct rdes* ptr = rdes; ptr <= &rdes[ROOMS_MAX-1]; ptr++) {
    for (int i = 0; i < ROOMS_MAX; i++) {
      ptr->isconn[i] = false;
    }
    ptr->ingraph = false;
  }

  /* starting with one room, connect it to a random adjacent room and
   * then pick a new room to start with.  */
  int roomcount = 1;
  struct rdes* r1 = &rdes[os_rand_range(ROOMS_MAX)];
  r1->ingraph = true;

  struct rdes* r2 = nullptr;
  do {
      /* find a room to connect with */
      int j = 0;
      for (int i = 0; i < ROOMS_MAX; i++) {
        if (r1->conn[i] && !rdes[i].ingraph && !os_rand_range(++j)) {
          r2 = &rdes[i];
        }
      }

      /* if no adjacent rooms are outside the graph, pick a new room
       * to look from */
      if (j == 0) {
        do {
          r1 = &rdes[os_rand_range(ROOMS_MAX)];
        } while (!r1->ingraph);

      /* otherwise, connect new room to the graph, and draw a tunnel
       * to it */
      } else {
        r2->ingraph = true;
        int i = static_cast<int>(r1 - rdes);
        j = static_cast<int>(r2 - rdes);
        connect_passages(i, j);
        r1->isconn[j] = true;
        r2->isconn[i] = true;
        roomcount++;
      }
    } while (roomcount < ROOMS_MAX);

    /* attempt to add passages to the graph a random number of times so
     * that there isn't always just one unique passage through it.  */
  for (roomcount = os_rand_range(5); roomcount > 0; roomcount--) {

    r1 = &rdes[os_rand_range(ROOMS_MAX)];	/* a random room to look from */

    /* find an adjacent room not already connected */
    int j = 0;
    for (int i = 0; i < ROOMS_MAX; i++) {
      if (r1->conn[i] && !r1->isconn[i] && os_rand_range(++j) == 0) {
        r2 = &rdes[i];
      }
    }

    /* if there is one, connect it and look for the next added passage */
    if (j != 0) {
      int i = static_cast<int>(r1 - rdes);
      j = static_cast<int>(r2 - rdes);
      connect_passages(i, j);
      r1->isconn[j] = true;
      r2->isconn[i] = true;
    }
  }

  /* Assign a number to each passageway */
  for (room& passage : passages) {
    passage.r_nexits = 0;
  }

  pnum = 0;
  newpnum = false;
  for (int i = 0; i < ROOMS_MAX; ++i) {
    for (int j = 0; j < rooms[i].r_nexits; ++j) {
      newpnum = true;
      number_passage(rooms[i].r_exit[j].x, rooms[i].r_exit[j].y);
    }
  }
}

void Level::place_passage(Coordinate* coord) {

  if (coord == nullptr) {
    error("coord was null");
  }

  set_passage(*coord);

  if (os_rand_range(10) + 1 < Game::current_level && os_rand_range(40) == 0) {
    set_not_real(*coord);
  } else {
    set_ch(*coord, PASSAGE);
  }
}

void
Level::wizard_show_passages() {

  for (int y = 1; y < NUMLINES - 1; y++) {
    for (int x = 0; x < NUMCOLS; x++) {

      char ch = get_ch(x, y);

      if (is_passage(x, y) || ch == DOOR ||
          (!is_real(x, y) && (ch == VWALL || ch == HWALL))) {
        if (is_passage(x, y)) {
          ch = PASSAGE;
        }
        set_discovered(x, y);
        move(y, x);
        Monster *mon = get_monster(x, y);
        if (mon != nullptr) {
          mon->t_oldch = ch;
        } else if (is_real(x, y)) {
          addcch(static_cast<chtype>(ch));
        } else {
          standout();
          addcch(is_passage(x, y) ? PASSAGE : DOOR);
          standend();
        }
      }
    }
  }
}
