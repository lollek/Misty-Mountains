#include <ctype.h>
#include <assert.h>

#include "error_handling.h"
#include "game.h"
#include "coordinate.h"
#include "scrolls.h"
#include "command_private.h"
#include "command.h"
#include "traps.h"
#include "io.h"
#include "armor.h"
#include "fight.h"
#include "monster.h"
#include "move.h"
#include "level_rooms.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "options.h"
#include "level.h"
#include "os.h"
#include "rogue.h"

using namespace std;

static void stop_on_interesting_stuff(Coordinate const& nh, int dx, int dy) {
  if (!door_stop) {
    return;
  }

  // Check if we have reached some crossroads (if tunnel)
  if (dx == 0 && dy != 0) {
    if ((Game::level->get_tile(nh.x -1, nh.y - dy) == Tile::Wall &&
         Game::level->get_tile(nh.x -1, nh.y)      != Tile::Wall) ||
        (Game::level->get_tile(nh.x +1, nh.y - dy) == Tile::Wall &&
         Game::level->get_tile(nh.x +1, nh.y     ) != Tile::Wall)) {
      player->set_not_running();
      return;
    }
  } else if (dx != 0 && dy == 0) {
    if ((Game::level->get_tile(nh.x - dx, nh.y -1) == Tile::Wall &&
         Game::level->get_tile(nh.x,      nh.y -1) != Tile::Wall) ||
        (Game::level->get_tile(nh.x - dx, nh.y +1) == Tile::Wall &&
         Game::level->get_tile(nh.x,      nh.y +1) != Tile::Wall)) {
      player->set_not_running();
      return;
    }
  }


  for (int x = nh.x -1; x <= nh.x +1; ++x) {
    for (int y = nh.y -1; y <= nh.y +1; ++y) {

      Monster* monster = Game::level->get_monster(x, y);
      // Monster are interesting, and they will notice you as well
      if (monster != nullptr) {
        monster_notice_player(y, x);

        // Only actually notice it if we can see it
        if (player->can_sense_monsters() || !monster->is_invisible()) {
          player->set_not_running();
        }
        return;
      }

      // Items are interesting
      Item *item = Game::level->get_item(x, y);
      if (item != nullptr) {
        player->set_not_running();
        return;
      }

      // Always stop near traps and stairs, and doors IFF we can actually move
      // to it (not diagonal)
      Tile::Type tile = Game::level->get_tile(x, y);
      Coordinate tile_coord(x, y);
      if ((tile == Tile::Stairs || tile == Tile::Trap || tile == Tile::OpenDoor)) {
        player->set_not_running();
        return;
      }
    }
  }
}

/** move_turn_ok:
 * Decide whether it is legal to turn onto the given space */
static bool
move_turn_ok(int y, int x)
{
  switch (Game::level->get_tile(x, y)) {
    case Tile::Floor: case Tile::OpenDoor: case Tile::Stairs:
      return true;

    case Tile::Wall: case Tile::Trap: case Tile::ClosedDoor:
      return false;
  }
}

/** move_turnref:
 * Decide whether to refresh at a passage turning or not */
static void
move_turnref(void)
{
  if (!Game::level->is_discovered(player->get_position())) {
    if (jump) {
      leaveok(stdscr, true);
      refresh();
      leaveok(stdscr, false);
    }
    Game::level->set_discovered(player->get_position());
  }
}

static bool
move_do_loop_wall(bool& after, int& dx, int& dy) {
  if (passgo && player->is_running()) {


    if (dx != 0 && dy == 0) {
      Coordinate const* player_pos = &player->get_position();
      bool b1 = (player_pos->y != 1 && move_turn_ok(player_pos->y - 1, player_pos->x));
      bool b2 = (player_pos->y != NUMLINES - 2 && move_turn_ok(player_pos->y + 1, player_pos->x));
      if (!(b1 ^ b2)) {
        return false;
      }

      if (b1) {
        runch = 'k';
        dy = -1;

      } else {
        runch = 'j';
        dy = 1;
      }

      dx = 0;
      move_turnref();
      return true;

    } else if (dx == 0 && dy != 0) {
      Coordinate const* player_pos = &player->get_position();
      bool b1 = (player_pos->x != 0 && move_turn_ok(player_pos->y, player_pos->x - 1));
      bool b2 = (player_pos->x != NUMCOLS - 1 && move_turn_ok(player_pos->y, player_pos->x + 1));
      if (!(b1 ^ b2)) {
        return false;
      }

      if (b1) {
        runch = 'h';
        dx = -1;

      } else {
        runch = 'l';
        dx = 1;
      }

      dy = 0;
      move_turnref();
      return true;
    }
  }
  player->set_not_running();
  after = false;
  return false;
}

static bool
move_do_loop_default(bool after, Coordinate& coord) {

  // If there was a monster there, fight it!
  if (Game::level->get_monster(coord) != nullptr) {
    fight_against_monster(&coord, player->equipped_weapon(), false);
    return after;
  }

  // If there was a trap, get trapped!
  if (Game::level->get_tile(coord) == Tile::Trap) {
    char ch = Trap::player(coord);
    if (ch == Trap::Door || ch == Trap::Teleport) {
      return after;
    }
  }

  // Else, Move player
  Coordinate old_position = player->get_position();
  player->set_room(Game::level->get_room(old_position));
  player->set_position(coord);

  // Try to pick up any items here
  Item *item = Game::level->get_item(coord);
  if (item != nullptr) {
    command_pick_up(false);
    player->set_not_running();
  }

  player->set_room(Game::level->get_room(coord));

  return after;
}

static bool
move_do_loop(int dx, int dy) {
  bool loop = true;
  bool after = true;

  // If passgo is enabled and we are running, this function will loop until we
  // reach something of interest. See move_do_loop_wall for this
  while (loop) {

    Coordinate nh;
    nh.y = player->get_position().y + dy;
    nh.x = player->get_position().x + dx;

    // Stop running if we change rooms
    if (Game::level->get_room(nh) != player->get_room()) {
      player->set_not_running();
    }

    // If we are too close to the edge of map, treat is as wall automatically
    if (nh.x < 1 || nh.x >= NUMCOLS -1 || nh.y < 1 || nh.y >= NUMLINES - 1) {
      loop = move_do_loop_wall(after, dx, dy);
      continue;
    }

    // If we end up in the same position, stop so we don't deadlock
    if (player->is_running() && player->get_position() == nh) {
      after = false;
      player->set_not_running();
    }

    // Run a check if player wants to stop running
    if (player->is_running()) {
      stop_on_interesting_stuff(nh, dx, dy);
    }

    // Cannot escape from the flytrap
    Monster* mon = Game::level->get_monster(nh);
    if (player->is_held()) {
      if (mon == nullptr || mon->get_type() != 'F') {
        Game::io->message("you are being held");
        return after;
      }
    }

    // If it was a trap there, try to spring it!
    // this will trigger for real in the move_do_loop_default function
    Tile::Type ch = Game::level->get_tile(nh);
    if (!Game::level->is_real(nh) && ch == Tile::Floor && !player->is_levitating()) {
      ch = Tile::Trap;
      Game::level->set_tile(nh, Tile::Trap);
      Game::level->set_real(nh);
    }

    switch (ch) {
      case Tile::Wall:
      case Tile::ClosedDoor:
        loop = move_do_loop_wall(after, dx, dy); break;

      case Tile::Floor:
      case Tile::Stairs:
      case Tile::Trap:
      case Tile::OpenDoor:
        return move_do_loop_default(after, nh);
    }
  }
  return after;
}



bool
move_do(char ch) {
  int dy = 0, dx = 0;

  switch (ch) {
    case 'h': dy =  0; dx = -1; break;
    case 'j': dy =  1; dx =  0; break;
    case 'k': dy = -1; dx =  0; break;
    case 'l': dy =  0, dx =  1; break;
    case 'y': dy = -1, dx = -1; break;
    case 'u': dy = -1, dx =  1; break;
    case 'b': dy =  1, dx = -1; break;
    case 'n': dy =  1, dx =  1; break;
  }

  // If we cannot really move, return
  if (player_turns_without_moving) {
    player_turns_without_moving--;
    Game::io->message("you are still stuck in the bear trap");
    return true;
  }

  // If we are confused, we don't decide ourselves where to stumble
  if (player->is_confused() && os_rand_range(5) != 0) {

    Coordinate nh = player->possible_random_move();
    if (nh == player->get_position()) {
      player->set_not_running();
      to_death = false;
      return false;
    }
    dx = nh.x - player->get_position().x;
    dy = nh.y - player->get_position().y;

  }
  return move_do_loop(dx, dy);
}

