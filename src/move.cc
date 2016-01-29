#include <ctype.h>
#include <assert.h>

#include "error_handling.h"
#include "game.h"
#include "coordinate.h"
#include "scrolls.h"
#include "command.h"
#include "traps.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
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

Coordinate move_pos_prev;

/** move_turn_ok:
 * Decide whether it is legal to turn onto the given space */
static bool
move_turn_ok(int y, int x)
{
  return (Game::level->get_ch(x, y) == DOOR
      || (Game::level->is_real(x, y) && Game::level->is_passage(x, y)));
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
  if (passgo && player->is_running() && (player->get_room()->r_flags & ISGONE) &&
      !player->is_blind()) {


    if (runch == 'h' || runch == 'l') {
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

    } else if (runch == 'j' || runch == 'k') {
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
move_do_loop_door(bool after, Coordinate& coord) {
  player->set_not_running();

  Coordinate player_old_pos = player->get_position();
  if (Game::level->is_passage(player_old_pos)) {
    room_enter(coord);
  }

  player->set_position(coord);
  Game::io->print_tile(player_old_pos);
  return after;
}

static bool
move_do_loop_trap(bool after, Coordinate& coord) {
  char ch = trap_spring(nullptr, &coord);

  if (ch == T_DOOR || ch == T_TELEP) {
    return after;
  }

  if (Game::level->is_passage(coord) && Game::level->get_ch(move_pos_prev) == DOOR) {
    room_leave(coord);
  }

  player->set_position(coord);
  return after;
}

static bool
move_do_loop_passage(bool after, Coordinate& coord) {
  // when you're in a corridor, you don't know if you're in
  // a maze room or not, and there ain't no way to find out
  // if you're leaving a maze room, so it is necessary to
  // always recalculate which room player is in.

  player->set_room(Game::level->get_room(player->get_position()));

  char previous_place = Game::level->get_ch(player->get_position());
  player->set_position(coord);

  if (previous_place == DOOR) {
    room_leave(coord);
  }

  return after;
}

static bool
move_do_loop_floor(bool after, Coordinate& coord) {

  player->set_position(coord);

  // Try to pick up item, if we are standing on one
  Item *item = Game::level->get_item(coord);
  if (item != nullptr) {
    pack_pick_up(item, false);
    player->set_not_running();
  }

  return after;
}

static bool
move_do_loop_default(bool after, Coordinate& coord) {
  player->set_not_running();

  // Fight the monster, if there
  if (Game::level->get_monster(coord) != nullptr) {
    fight_against_monster(&coord, pack_equipped_item(EQUIPMENT_RHAND), false);
    return after;
  }

  // Move player
  player->set_position(coord);

  // Reprint (basically hide) old room, if we leave one
  if (Game::level->is_passage(coord) && Game::level->get_ch(move_pos_prev) == DOOR) {
    room_leave(coord);
  }

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

    // If we are too close to the edge of map, treat is as wall automatically
    if (nh.x < 1 || nh.x >= NUMCOLS -1 || nh.y < 1 || nh.y >= NUMLINES - 1) {
      loop = move_do_loop_wall(after, dx, dy);
      continue;
    }

    // Refuse illegal moves
    if (!diag_ok(&player->get_position(), &nh)) {
      player->set_not_running();
      return false;
    }

    // If we end up in the same position, stop so we don't deadlock
    if (player->is_running() && player->get_position() == nh) {
      after = false;
      player->set_not_running();
    }

    // If we are near something of interest, stop running
    for (int x = nh.x -1; x <= nh.x +1; ++x) {
      for (int y = nh.y -1; y <= nh.y +1; ++y) {
        char tile_ch = Game::level->get_ch(x, y);
        Monster* monster = Game::level->get_monster(x, y);
        Coordinate tile_coord(x, y);

        // Monster (also makes monster notice player)
        if (monster != nullptr) {
          player->set_not_running();
          monster_notice_player(y, x);
        }

        else if (
            // Item
            Game::level->get_item(x, y) != nullptr ||
            // Stairs or Trap
            tile_ch == STAIRS || tile_ch == TRAP ||
            // Door IF we can actually move to it
            (tile_ch == DOOR && diag_ok(&nh, &tile_coord))) {
          player->set_not_running();
        }
      }
    }


    // Cannot escape from the flytrap
    Monster* mon = Game::level->get_monster(nh);
    if (player->is_held()) {
      if (mon == nullptr || mon->get_type() != 'F') {
        io_msg("you are being held");
        return after;
      }
    }

    // If it was a trap there, try to spring it
    char ch = Game::level->get_type(nh);
    if (!Game::level->is_real(nh) && ch == FLOOR && !player->is_levitating()) {
      ch = TRAP;
      Game::level->set_ch(nh, TRAP);
      Game::level->set_real(nh);
    }

    switch (ch) {
      case SHADOW:   loop = move_do_loop_wall(after, dx, dy); break;
      case VWALL:    loop = move_do_loop_wall(after, dx, dy); break;
      case HWALL:    loop = move_do_loop_wall(after, dx, dy); break;
      case DOOR:     return move_do_loop_door(after, nh);
      case TRAP:     return move_do_loop_trap(after, nh);
      case PASSAGE:  return move_do_loop_passage(after, nh);
      case FLOOR:    return move_do_loop_floor(after, nh);
      default:       return move_do_loop_default(after, nh);
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
  firstmove = false;
  if (player_turns_without_moving) {
    player_turns_without_moving--;
    io_msg("you are still stuck in the bear trap");
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

