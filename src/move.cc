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
      || (Game::level->get_flag_real(x, y) && Game::level->get_flag_passage(x, y)));
}

/** move_turnref:
 * Decide whether to refresh at a passage turning or not */
static void
move_turnref(void)
{
  Coordinate *player_pos = player_get_pos();

  if (!Game::level->get_flag_seen(*player_pos)) {
    if (jump) {
      leaveok(stdscr, true);
      refresh();
      leaveok(stdscr, false);
    }
    Game::level->set_flag_seen(*player_pos);
  }
}

static bool
move_do_loop_wall(bool& after, int& dx, int& dy) {
  if (passgo && running && (player_get_room()->r_flags & ISGONE) &&
      !player_is_blind()) {


    if (runch == 'h' || runch == 'l') {
      Coordinate *player_pos = player_get_pos();
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
      Coordinate *player_pos = player_get_pos();
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
  running = false;
  after = false;
  return false;
}

static bool
move_do_loop_door(bool after, Coordinate& coord, bool is_passage) {
  Coordinate *player_pos = player_get_pos();
  running = false;

  if (Game::level->get_flag_passage(*player_pos)) {
    room_enter(&coord);
  }

  mvaddcch(player_pos->y, player_pos->x, static_cast<chtype>(floor_at()));

  if (is_passage && Game::level->get_ch(move_pos_prev) == DOOR) {
    room_leave(&coord);
  }

  player_set_pos(&coord);
  return after;
}

static bool
move_do_loop_trap(bool after, Coordinate& coord, bool is_passage) {
  Coordinate *player_pos = player_get_pos();
  char ch = trap_spring(nullptr, &coord);

  if (ch == T_DOOR || ch == T_TELEP) {
    return after;
  }

  mvaddcch(player_pos->y, player_pos->x, static_cast<chtype>(floor_at()));
  if (is_passage && Game::level->get_ch(move_pos_prev) == DOOR) {
    room_leave(&coord);
  }

  player_set_pos(&coord);
  return after;
}

static bool
move_do_loop_passage(bool after, Coordinate& coord, bool is_passage) {
  // when you're in a corridor, you don't know if you're in
  // a maze room or not, and there ain't no way to find out
  // if you're leaving a maze room, so it is necessary to
  // always recalculate which room player is in.

  Coordinate *player_pos = player_get_pos();
  player_set_room(Game::level->get_room(*player_pos));
  mvaddcch(player_pos->y, player_pos->x, static_cast<chtype>(floor_at()));

  if (is_passage && Game::level->get_ch(move_pos_prev) == DOOR) {
    room_leave(&coord);
  }

  player_set_pos(&coord);

  return after;
}

static bool
move_do_loop_floor(bool after, Coordinate& coord, bool is_passage, bool is_real) {
  Coordinate *player_pos = player_get_pos();
  if (!is_real) {
    trap_spring(nullptr, &coord);
  }

  mvaddcch(player_pos->y, player_pos->x, static_cast<chtype>(floor_at()));
  if (is_passage && Game::level->get_ch(move_pos_prev) == DOOR) {
    room_leave(&coord);
  }

  player_set_pos(&coord);
  return after;
}

static bool
move_do_loop_default(char ch, bool after, Coordinate& coord, bool is_passage) {
  running = false;
  if (isupper(ch) || Game::level->get_monster(coord)) {
    fight_against_monster(&coord, pack_equipped_item(EQUIPMENT_RHAND), false);

  } else {
    Coordinate *player_pos = player_get_pos();
    mvaddcch(player_pos->y, player_pos->x, static_cast<chtype>(floor_at()));

    if (is_passage && Game::level->get_ch(move_pos_prev) == DOOR) {
      room_leave(&coord);
    }

    player_set_pos(&coord);
    if (ch != STAIRS) {

      Item *item = Game::level->get_item(coord);
      if (item == nullptr) {
        error("No item at position");
      }
      pack_pick_up(item, false);
    }
  }

  return after;
}

static bool
move_do_loop(char ch, int dx, int dy) {
  bool loop = true;
  bool after = true;

  // If passgo is enabled and we are running, this function will loop until we
  // reach something of interest. See move_do_loop_wall for this
  while (loop) {

    Coordinate nh;
    nh.y = player_get_pos()->y + dy;
    nh.x = player_get_pos()->x + dx;

    /* Check if he tried to move off the screen or make an illegal
     * diagonal move, and stop him if he did. */
    if (nh.x < 0 || nh.x >= NUMCOLS || nh.y <= 0 || nh.y >= NUMLINES - 1) {
      loop = move_do_loop_wall(after, dx, dy);
      continue;
    }

    if (!diag_ok(player_get_pos(), &nh)) {
      running = false;
      return false;
    }

    if (running && *player_get_pos() == nh) {
      after = running = false;
    }

    bool is_passage = Game::level->get_flag_passage(nh);
    bool is_real    = Game::level->get_flag_real(nh);
    ch = Game::level->get_type(nh);

    if (!Game::level->get_flag_real(nh) && ch == FLOOR) {
      if (!player_is_levitating()) {
        ch = TRAP;
        Game::level->set_ch(nh, ch);
        Game::level->set_flag_real(nh);
      }

    } else if (player_is_held() && ch != 'F') {
      io_msg("you are being held");
      return after;
    }

    switch (ch) {
      case SHADOW:   loop = move_do_loop_wall(after, dx, dy); break;
      case VWALL:    loop = move_do_loop_wall(after, dx, dy); break;
      case HWALL:    loop = move_do_loop_wall(after, dx, dy); break;
      case DOOR:     return move_do_loop_door(after, nh, is_passage);
      case TRAP:     return move_do_loop_trap(after, nh, is_passage);
      case PASSAGE:  return move_do_loop_passage(after, nh, is_passage);
      case FLOOR:    return move_do_loop_floor(after, nh, is_passage, is_real);
      default:       return move_do_loop_default(ch, after, nh, is_passage);
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
  if (player_is_confused() && os_rand_range(5) != 0) {

    Coordinate nh;
    move_random(player, &nh);
    if (nh == *player_get_pos()) {
      running = false;
      to_death = false;
      return false;
    }
    return move_do_loop(ch, 0, 0);

  } else {
    return move_do_loop(ch, dx, dy);
  }
}

/** move_random:
 * Move in a random direction if the monster/person is confused */
void
move_random(Monster* who, Coordinate* ret)
{
  assert(who != nullptr);

  /* Now check to see if that's a legal move.
   * If not, don't move.(I.e., bump into the wall or whatever) */
  int x = ret->x = who->t_pos.x + os_rand_range(3) - 1;
  int y = ret->y = who->t_pos.y + os_rand_range(3) - 1;
  if (y == who->t_pos.y && x == who->t_pos.x)
    return;

  if (!diag_ok(&who->t_pos, ret))
  {
    ret->x = who->t_pos.x;
    ret->y = who->t_pos.y;
    return;
  }

  char ch = Game::level->get_type(x, y);
  if (!step_ok(ch))
  {
    ret->x = who->t_pos.x;
    ret->y = who->t_pos.y;
    return;
  }

  if (ch == SCROLL)
  {
    Item* item = Game::level->get_item(x, y);
    if (item != nullptr && item->o_which == S_SCARE) {
      ret->x = who->t_pos.x;
      ret->y = who->t_pos.y;
      return;
    }
  }
}

