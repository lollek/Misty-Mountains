#include <exception>
#include <string>

#include "coordinate.h"
#include "death.h"
#include "error_handling.h"
#include "fight.h"
#include "game.h"
#include "io.h"
#include "item/weapons.h"
#include "level.h"
#include "misc.h"
#include "monster.h"
#include "os.h"
#include "player.h"
#include "rogue.h"

#include "magic.h"

using namespace std;

static bool magic_bolt_handle_bounces(Coordinate& pos, Coordinate* dir,
                                      char* dirtile) {
  int num_bounces{0};
recursive_loop:; /* ONLY called by end of function */
  Monster* monster{Game::level->get_monster(pos)};
  Tile::Type const ch{Game::level->get_tile(pos)};
  if (monster != nullptr || ch != Tile::Wall) {
    return num_bounces != 0;
  }

  // There are no known bugs with this functions at the moment,
  // but just in case, we'll abort after too many bounces
  if (num_bounces > 10) {
    return true;
  }

  // Handle potential bouncing
  if (ch == Tile::Wall) {
    enum bounce_type_t { Horizontal, Vertical } bounce_type;

    if (dir->x != 0 && dir->y == 0) {
      bounce_type = Vertical;

    } else if (dir->y != 0 && dir->x == 0) {
      bounce_type = Horizontal;

    } else {
      int const y{dir->y < 0 ? pos.y + 1 : pos.y - 1};
      Tile::Type y_ch;
      if (y >= IO::map_height || y <= 0)
        y_ch = Tile::Wall;
      else
        y_ch = Game::level->get_tile(pos.x, y);

      bounce_type = y_ch == Tile::Wall ? Vertical : Horizontal;
    }

    if (bounce_type == Vertical) {
      pos.x -= dir->x;
      dir->x = -dir->x;
    } else {
      pos.y -= dir->y;
      dir->y = -dir->y;
    }
  }

  if (*dirtile == IO::DiagonalDownBolt)
    *dirtile = IO::DiagonalUpBolt;
  else if (*dirtile == IO::DiagonalUpBolt)
    *dirtile = IO::DiagonalDownBolt;

  /* It's possible for a bolt to bounce directly from one wall to another
   * if you hit a corner, thus, we need to go through everything again. */
  ++num_bounces;
  goto recursive_loop; /* == magic_bolt_handle_bounces(pos,dir,dirtile) */
}

static void magic_bolt_hit_player(Coordinate* start,
                                  string const& missile_name) {
  if (start == nullptr) {
    error("start coord was null");
  }

  if (!player->saving_throw(VS_MAGIC)) {
    player->take_damage(roll(6, 6));
    if (player->get_health() <= 0) {
      if (start == &player->get_position())
        switch (missile_name[0]) {
          case 'f': death(DEATH_FLAME);
          case 'i': death(DEATH_ICE);
          default: death(DEATH_UNKNOWN);
        }
      else
        death(Game::level->get_monster(*start)->get_subtype());
    }
    Game::io->message("you are hit by the " + missile_name);
  } else
    Game::io->message("the " + missile_name + " whizzes by you");
}

static void magic_bolt_hit_monster(Monster* mon, Coordinate* start,
                                   Coordinate* pos,
                                   string const& missile_name) {
  if (mon == nullptr) {
    error("mon was null");
  } else if (start == nullptr) {
    error("start was null");
  } else if (pos == nullptr) {
    error("pos was null");
  }

  if (!monster_save_throw(VS_MAGIC, mon)) {
    class Weapon bolt(Weapon::Spear);
    bolt.set_hit_plus(100);
    bolt.set_damage_plus(0);
    bolt.set_throw_damage({6, 6});
    bolt.set_position(*pos);

    if (mon->get_look() == 'D' && missile_name == "flame") {
      Game::io->message("the flame bounces off the dragon");
    } else {
      fight_against_monster(pos, &bolt, true, &missile_name);
    }
  }
}

void magic_bolt(Coordinate* start, Coordinate* dir, string const& name) {
  if (start == nullptr) {
    error("start was null");
  } else if (dir == nullptr) {
    error("dir was null");
  }

  char dirtile{'?'};
  switch (dir->y + dir->x) {
    case 0: dirtile = IO::DiagonalUpBolt; break;
    case 1:
    case -1:
      dirtile = (dir->y == 0 ? IO::HorizontalBolt : IO::VerticalBolt);
      break;
    case 2:
    case -2: dirtile = IO::DiagonalDownBolt; break;
  }

  IO::Attribute color = IO::Attribute::Red;
  if (name == "ice") {
    color = IO::Attribute::Blue;
  }

  Coordinate pos = *start;
  for (int i{0}; i < BOLT_LENGTH; ++i) {
    pos.y += dir->y;
    pos.x += dir->x;

    if (magic_bolt_handle_bounces(pos, dir, &dirtile))
      Game::io->message("the " + name + " bounces");

    /* Handle potential hits */
    if (pos == player->get_position())
      magic_bolt_hit_player(start, name);

    Monster* tp{Game::level->get_monster(pos)};
    if (tp != nullptr) {
      magic_bolt_hit_monster(tp, start, &pos, name);
    }

    Game::io->print(pos.x, pos.y, static_cast<long unsigned int>(dirtile),
                    color);
  }

  Game::io->force_redraw();
  os_usleep(200000);
}
