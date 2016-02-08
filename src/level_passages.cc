#include <cstdlib>
#include <exception>

#include "error_handling.h"
#include "game.h"
#include "coordinate.h"
#include "io.h"
#include "os.h"
#include "level_rooms.h"
#include "rogue.h"

#include "level.h"

using namespace std;

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
    set_tile(*coord, Tile::Wall);
    set_not_real(*coord);
  } else {
    set_tile(*coord, Tile::Door);
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

  room* room_from = &rooms.at(static_cast<size_t>(rm));
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
    room_to = &rooms.at(static_cast<size_t>(rm + 3));
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

    room_to = &rooms.at(static_cast<size_t>(rm + 1));
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
    Game::io->message("DEBUG: error in connection tables");
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


// TODO: Clean up this function. Pretty hacky
void
Level::create_passages()
{
  struct Destination {
    bool operator==(Destination* d) { return this == d; }
    vector<bool> conn;  /* possible to connect to room i? */
    vector<bool> isconn;/* connection been made to room i? */
    bool ingraph;         /* this room in graph already? */
  };

  passages.clear();

  vector<Destination> destinations {
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

  if (destinations.size() != 9) {
    error("Expected 9 rooms in destinations");
  }

  // reinitialize room graph description
  for (Destination& ptr : destinations) {
    if (ptr.isconn.size() != 9) {
      error("Expected 3 isconns in ptr");
    }

    for (size_t i = 0; i < ptr.isconn.size(); i++) {
      ptr.isconn.at(i) = false;
    }
    ptr.ingraph = false;
  }

  // starting with one room, connect it to a random adjacent room and
  // then pick a new room to start with.
  size_t roomcount = 1;
  Destination* r1 = &destinations.at(os_rand_range(destinations.size()));
  r1->ingraph = true;

  Destination* r2 = nullptr;
  do {
      /* find a room to connect with */
      int j = 0;
      for (size_t i = 0; i < destinations.size(); i++) {
        if (r1->conn.at(i) &&
            !destinations.at(i).ingraph &&
            !os_rand_range(++j)) {
          r2 = &destinations.at(i);
        }
      }

      /* if no adjacent rooms are outside the graph, pick a new room
       * to look from */
      if (j == 0) {
        do {
          r1 = &destinations.at(os_rand_range(destinations.size()));
        } while (!r1->ingraph);

      /* otherwise, connect new room to the graph, and draw a tunnel
       * to it */
      } else {
        if (r2 == nullptr) {
          error("r2 was null");
        }

        r2->ingraph = true;
        size_t r1i = static_cast<size_t>(find(destinations.begin(), destinations.end(), r1)
            - destinations.begin());
        size_t r2i = static_cast<size_t>(find(destinations.begin(), destinations.end(), r2)
            - destinations.begin());
        if (r1i >= destinations.size() || r2i >= destinations.size()) {
          error("i or j was not found in destinations");
        }
        connect_passages(static_cast<int>(r1i), static_cast<int>(r2i));
        r1->isconn.at(r1i) = true;
        r2->isconn.at(r2i) = true;
        roomcount++;
      }
    } while (roomcount < rooms.size());

    /* attempt to add passages to the graph a random number of times so
     * that there isn't always just one unique passage through it.  */
  for (roomcount = static_cast<size_t>(os_rand_range(5)); roomcount > 0; roomcount--) {

    r1 = &destinations.at(os_rand_range(destinations.size()));	/* a random room to look from */

    /* find an adjacent room not already connected */
    int j = 0;
    for (size_t i = 0; i < destinations.size(); i++) {
      if (r1->conn.at(i) && !r1->isconn.at(i) && os_rand_range(++j) == 0) {
        r2 = &destinations.at(i);
      }
    }

    /* if there is one, connect it and look for the next added passage */
    if (j != 0) {
      size_t r1i = static_cast<size_t>(find(destinations.begin(), destinations.end(), r1)
          - destinations.begin());
      size_t r2i = static_cast<size_t>(find(destinations.begin(), destinations.end(), r2)
          - destinations.begin());
      if (r1i >= destinations.size() || r2i >= destinations.size()) {
        error("i or j was not found in destinations");
      }
      connect_passages(static_cast<int>(r1i), static_cast<int>(r2i));
      r1->isconn.at(r1i) = true;
      r2->isconn.at(r2i) = true;
    }
  }

  /* Assign a number to each passageway */
  for (room& passage : passages) {
    passage.r_nexits = 0;
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
    set_tile(*coord, Tile::Floor);
  }
}

void
Level::wizard_show_passages() {

  for (int y = 1; y < NUMLINES - 1; y++) {
    for (int x = 0; x < NUMCOLS; x++) {

      Tile::Type ch = get_tile(x, y);

      if (is_passage(x, y) || ch == Tile::Door ||
          (!is_real(x, y) && ch == Tile::Wall)) {
        if (is_passage(x, y)) {
          ch = Tile::Floor;
        }
        set_discovered(x, y);
        if (is_real(x, y)) {
          Game::io->print_color(x, y, ch);
        } else {
          standout();
          Game::io->print_color(x, y, is_passage(x, y) ? Tile::Floor : Tile::Door);
          standend();
        }
      }
    }
  }
}
