#include <exception>

using namespace std;

#include "game.h"
#include "coordinate.h"
#include "io.h"
#include "pack.h"
#include "monster.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "os.h"
#include "rogue.h"

#include "level_rooms.h"

room* room_prev;
room rooms[ROOMS_MAX];

/* position matrix for maze positions */
struct spot {
  int        nexits;
  Coordinate exits[4];
  int        used;
};

static spot maze[NUMLINES/3+1][NUMCOLS/3+1];

/* Called to illuminate a room.
 * If it is dark, remove anything that might move.  */
static void
room_open_door(struct room* rp) {
  if (rp->r_flags & ISGONE) {
    return;
  }

  for (int y = rp->r_pos.y; y < rp->r_pos.y + rp->r_max.y; y++) {
    for (int x = rp->r_pos.x; x < rp->r_pos.x + rp->r_max.x; x++) {
      if (isupper(Game::level->get_type(x, y))) {
        monster_notice_player(y, x);
      }
    }
  }
}

/* Account for maze exits */
static void
room_accnt_maze(int y, int x, int ny, int nx) {
  spot* sp = &maze[y][x];
  Coordinate* cp;

  for (cp = sp->exits; cp < &sp->exits[sp->nexits]; cp++) {
    if (cp->y == ny && cp->x == nx) {
      return;
    }
  }

  cp->y = ny;
  cp->x = nx;
}


/* Dig out from around where we are now, if possible */
void
Level::draw_maze_recursive(int y, int x, int starty, int startx, int maxy, int maxx) {
  int nexty = 0;
  int nextx = 0;

  for (;;) {
    Coordinate const del[4] = { {2, 0}, {-2, 0}, {0, 2}, {0, -2} };
    int cnt = 0;
    for (unsigned i = 0; i < sizeof(del)/sizeof(*del); ++i) {
      int newy = y + del[i].y;
      int newx = x + del[i].x;

      if (newy < 0 || newy > maxy || newx < 0 || newx > maxx
          || Game::level->get_flags(newx + startx, newy + starty) & F_PASS) {
        continue;
      }

      if (os_rand_range(++cnt) == 0) {
        nexty = newy;
        nextx = newx;
      }
    }

    if (cnt == 0) {
      return;
    }

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
    this->place_passage(&pos);

    pos.y = nexty + starty;
    pos.x = nextx + startx;
    this->place_passage(&pos);

    this->draw_maze_recursive(nexty, nextx, starty, startx, maxy, maxx);
  }
}

/* Dig a maze */
void
Level::draw_maze(room const& rp) {
  for (spot* sp = &maze[0][0]; sp <= &maze[NUMLINES / 3][NUMCOLS / 3]; sp++) {
    sp->used = false;
    sp->nexits = 0;
  }

  int y = (os_rand_range(rp.r_max.y) / 2) * 2;
  int x = (os_rand_range(rp.r_max.x) / 2) * 2;

  // TODO: Is this a typo/incorrect? maybe x + rp->r_pos.x
  Coordinate pos(y + rp.r_pos.x, y + rp.r_pos.y);
  this->place_passage(&pos);

  this->draw_maze_recursive(y, x, rp.r_pos.y, rp.r_pos.x, rp.r_max.y, rp.r_max.x);
}

/* Draw a box around a room and lay down the floor for normal
 * rooms; for maze rooms, draw maze. */
void
Level::draw_room(room const& rp) {

  /* Draw left + right side */
  for (int y = rp.r_pos.y + 1; y <= rp.r_max.y + rp.r_pos.y - 1; y++) {
    this->set_ch(rp.r_pos.x, y, VWALL);
    this->set_ch(rp.r_pos.x + rp.r_max.x - 1, y, VWALL);
  }

  /* Draw top + bottom side */
  for (int x = rp.r_pos.x; x <= rp.r_pos.x + rp.r_max.x - 1; x++) {
    this->set_ch(x, rp.r_pos.y, HWALL);
    this->set_ch(x, rp.r_pos.y + rp.r_max.y - 1, HWALL);
  }

  /* Put the floor down */
  for (int y = rp.r_pos.y + 1; y < rp.r_pos.y + rp.r_max.y - 1; y++) {
    for (int x = rp.r_pos.x + 1; x < rp.r_pos.x + rp.r_max.x - 1; x++) {
      this->set_ch(x, y, FLOOR);
    }
  }
}

static void
room_place_gone_room(Coordinate const* max_size, Coordinate const* top, room* room) {
  /** Place a gone room.  Make certain that there is a blank line
   * for passage drawing.  */
  do {
    room->r_pos.x = top->x + os_rand_range(max_size->x - 3) + 1;
    room->r_pos.y = top->y + os_rand_range(max_size->y - 2) + 1;
    room->r_max.x = -NUMCOLS;
    room->r_max.y = -NUMLINES;
  } while (!(room->r_pos.y > 0 && rooms->r_pos.y < NUMLINES-1));
}

void
Level::create_rooms() {

  /* maximum room size */
  Coordinate const bsze(NUMCOLS / 3, NUMLINES / 3);

  /* Clear things for a new level */
  for (int i = 0; i < ROOMS_MAX; ++i) {
    rooms[i].r_goldval = 0;
    rooms[i].r_nexits = 0;
    rooms[i].r_flags = 0;
  }

  /* Put the gone rooms, if any, on the level */
  int left_out = os_rand_range(4);
  for (int i = 0; i < left_out; i++) {
    rooms[room_random()].r_flags |= ISGONE;
  }

  /* dig and populate all the rooms on the level */
  for (int i = 0; i < ROOMS_MAX; i++) {
    /* Find upper left corner of box that this room goes in */
    Coordinate const top((i % 3) * bsze.x + 1, (i / 3) * bsze.y);

    if (rooms[i].r_flags & ISGONE) {
      room_place_gone_room(&bsze, &top, &rooms[i]);
      continue;
    }

    /* set room type */
    if (os_rand_range(10) < Level::current_level - 1) {
      rooms[i].r_flags |= ISDARK;  /* dark room */
      if (os_rand_range(15) == 0) {
        rooms[i].r_flags = ISMAZE; /* maze room */
      }
    }

    /* Find a place and size for a random room */
    if (rooms[i].r_flags & ISMAZE) {
      rooms[i].r_max.x = bsze.x - 1;
      rooms[i].r_max.y = bsze.y - 1;
      rooms[i].r_pos.x = top.x == 1 ? 0 : top.x ;
      rooms[i].r_pos.y = top.y;
      if (rooms[i].r_pos.y == 0) {
        rooms[i].r_pos.y++;
        rooms[i].r_max.y--;
      }

    } else {
      do {
        rooms[i].r_max.x = os_rand_range(bsze.x - 4) + 4;
        rooms[i].r_max.y = os_rand_range(bsze.y - 4) + 4;
        rooms[i].r_pos.x = top.x + os_rand_range(bsze.x - rooms[i].r_max.x);
        rooms[i].r_pos.y = top.y + os_rand_range(bsze.y - rooms[i].r_max.y);
      } while (rooms[i].r_pos.y == 0);
    }

    if (rooms[i].r_flags & ISMAZE) {
      this->draw_maze(rooms[i]);
    } else {
      this->draw_room(rooms[i]);
    }

    /* Put the gold in */
    if (os_rand_range(2) == 0 &&
        (!pack_contains_amulet() || Level::current_level >= Level::max_level_visited)) {
      Item *gold = new Item();
      gold->o_goldval = rooms[i].r_goldval = GOLDCALC;
      this->get_random_room_coord(&rooms[i], &rooms[i].r_gold, 0, false);
      gold->set_pos(rooms[i].r_gold);
      this->set_ch(rooms[i].r_gold, GOLD);
      gold->o_flags = ISMANY;
      gold->o_type = GOLD;
      level_items.push_back(gold);
    }

    /* Put the monster in */
    if (os_rand_range(100) < (rooms[i].r_goldval > 0 ? 80 : 25)) {
      Coordinate mp;
      this->get_random_room_coord(&rooms[i], &mp, 0, true);
      Monster* monster = new Monster();
      monster_list.push_back(monster);
      monster_new(monster, monster_random(false), &mp, &rooms[i]);
      monster_give_pack(monster);
      this->set_monster(mp, monster);
    }
  }
}

bool
Level::get_random_room_coord(room* room, Coordinate* coord, int tries, bool monst) {
  bool limited_tries = tries > 0;
  char compchar = 0;
  bool pickroom = room == nullptr;

  if (!pickroom) {
    compchar = ((room->r_flags & ISMAZE) ? PASSAGE : FLOOR);
  }

  for (;;) {
    if (limited_tries && tries-- == 0) {
      return false;
    }

    if (pickroom) {
      room = &rooms[room_random()];
      compchar = ((room->r_flags & ISMAZE) ? PASSAGE : FLOOR);
    }

    /* Pick a random position */
    coord->x = room->r_pos.x + os_rand_range(room->r_max.x - 2) + 1;
    coord->y = room->r_pos.y + os_rand_range(room->r_max.y - 2) + 1;

    PLACE* pp = this->get_place(*coord);
    if (monst) {
      if (pp->p_monst == nullptr && step_ok(pp->p_ch)) {
        return true;
      }
    } else if (pp->p_ch == compchar) {
      return true;
    }
  }
}

void
room_enter(Coordinate* cp) {

  struct room* rp = Game::level->get_room(*cp);
  player_set_room(rp);
  room_open_door(rp);

  if (!(rp->r_flags & ISDARK) && !player_is_blind()) {

    for (int y = rp->r_pos.y; y < rp->r_max.y + rp->r_pos.y; y++) {

      move(y, rp->r_pos.x);
      for (int x = rp->r_pos.x; x < rp->r_max.x + rp->r_pos.x; x++) {
        Monster* tp = Game::level->get_monster(x, y);
        char ch = Game::level->get_ch(x, y);

        if (tp == nullptr) {
          mvaddcch(y, x, static_cast<chtype>(ch));
          continue;
        }

        tp->t_oldch = ch;
        if (monster_seen_by_player(tp)) {
          mvaddcch(y, x, static_cast<chtype>(tp->t_disguise));
        } else if (player_can_sense_monsters()) {
          mvaddcch(y, x, static_cast<chtype>(tp->t_disguise)| A_STANDOUT);
        } else {
          mvaddcch(y, x, static_cast<chtype>(ch));
        }
      }
    }
  }
}

/** room_leave:
 * Code for when we exit a room */
void
room_leave(Coordinate* cp)
{
  struct room* rp = player_get_room();

  if (rp->r_flags & ISMAZE) {
    return;
  }

  char floor;
  if (rp->r_flags & ISGONE) {
    floor = PASSAGE;
  } else if (!(rp->r_flags & ISDARK) || player_is_blind()) {
    floor = FLOOR;
  } else {
    floor = SHADOW;
  }

  player_set_room(&passages[Game::level->get_flags(*cp) & F_PNUM]);
  for (int y = rp->r_pos.y; y < rp->r_max.y + rp->r_pos.y; y++) {
    for (int x = rp->r_pos.x; x < rp->r_max.x + rp->r_pos.x; x++) {
      move(y, x);
      char ch = static_cast<char>(incch());

      if (ch == FLOOR && floor == SHADOW) {
        mvaddcch(y, x, SHADOW);
      }

      /* Don't touch non-monsters */
      else if (!isupper(ch)) {
        continue;
      }

      if (player_can_sense_monsters()) {
        mvaddcch(y, x, static_cast<chtype>(ch) | A_STANDOUT);
      } else {
        mvaddcch(y, x, static_cast<chtype>(Game::level->get_ch(x, y) == DOOR ? DOOR : floor));
      }
    }
  }

  room_open_door(rp);
}

int
room_random(void) {
  int rm;

  do {
    rm = os_rand_range(ROOMS_MAX);
  } while (rooms[rm].r_flags & ISGONE);

  return rm;
}
