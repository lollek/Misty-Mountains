#include <assert.h>
#include <ctype.h>

#include "command.h"
#include "command_private.h"
#include "coordinate.h"
#include "error_handling.h"
#include "fight.h"
#include "game.h"
#include "io.h"
#include "item/armor.h"
#include "item/scrolls.h"
#include "level.h"
#include "level.h"
#include "level/rooms.h"
#include "misc.h"
#include "monster.h"
#include "move.h"
#include "options.h"
#include "os.h"
#include "player.h"
#include "rogue.h"
#include "traps.h"

using namespace std;

static void handle_surrounding(Coordinate const& nh, int dx, int dy,
                               bool cautious) {
  // Check if we have reached some crossroads (if tunnel)
  if (cautious) {
    if (dx == 0 && dy != 0) {
      if ((Game::level->get_tile(nh.x - 1, nh.y - dy) == Tile::Wall &&
           Game::level->get_tile(nh.x - 1, nh.y) != Tile::Wall) ||
          (Game::level->get_tile(nh.x + 1, nh.y - dy) == Tile::Wall &&
           Game::level->get_tile(nh.x + 1, nh.y) != Tile::Wall)) {
        player->set_not_running();
      }
    } else if (dx != 0 && dy == 0) {
      if ((Game::level->get_tile(nh.x - dx, nh.y - 1) == Tile::Wall &&
           Game::level->get_tile(nh.x, nh.y - 1) != Tile::Wall) ||
          (Game::level->get_tile(nh.x - dx, nh.y + 1) == Tile::Wall &&
           Game::level->get_tile(nh.x, nh.y + 1) != Tile::Wall)) {
        player->set_not_running();
      }
    }
  }

  for (int x{nh.x - 1}; x <= nh.x + 1; ++x) {
    for (int y{nh.y - 1}; y <= nh.y + 1; ++y) {
      Monster* monster{Game::level->get_monster(x, y)};
      // Monster are interesting, and they will notice you as well
      if (monster != nullptr) {
        monster->notice_player();

        // Only actually notice it if we can see it
        if (cautious &&
            (player->can_sense_monsters() || !monster->is_invisible())) {
          player->set_not_running();
          continue;
        }
      }

      // Items are interesting
      Item const* item{Game::level->get_item(x, y)};
      if (cautious && item != nullptr) {
        player->set_not_running();
        continue;
      }

      // Always stop near some object types
      Tile::Type const tile{Game::level->get_tile(x, y)};
      Coordinate tile_coord(x, y);
      if (cautious && (tile == Tile::StairsDown || tile == Tile::StairsUp ||
                       tile == Tile::Trap || tile == Tile::OpenDoor ||
                       tile == Tile::Shop)) {
        player->set_not_running();
        continue;
      }
    }
  }
}

static bool move_do_loop_default(Coordinate& coord) {
  // If there was a monster there, fight it!
  if (Game::level->get_monster(coord) != nullptr) {
    fight_against_monster(&coord, player->equipped_weapon(), false);
    return true;
  }

  // If there was a trap, get trapped!
  if (Game::level->get_tile(coord) == Tile::Trap) {
    Trap::Type const ch{Trap::player(coord)};
    if (ch == Trap::Door || ch == Trap::Teleport) {
      return true;
    }
  }

  // Else, Move player
  player->set_position(coord);

  // Try to pick up any items here
  Item* item{Game::level->get_item(coord)};
  if (item != nullptr) {
    command_pick_up(false);
    player->set_not_running();
  }

  return true;
}

static bool move_do_loop(int dx, int dy, bool cautious) {
  if (dx == 0 && dy == 0) {
    player->set_not_running();
    return true;
  }

  Coordinate nh{player->get_position().x + dx, player->get_position().y + dy};

  // If we are too close to the edge of map, treat is as wall automatically
  if (nh.x < 1 || nh.x >= IO::map_width - 1 || nh.y < 1 ||
      nh.y >= IO::map_height - 1) {
    player->set_not_running();
    return true;
  }

  // Run a check if player wants to stop running
  handle_surrounding(nh, dx, dy, cautious);

  if (player->is_held()) {
    Game::io->message("you are being held");
    return true;
  }

  // If it was a trap there, try to spring it!
  // this will trigger for real in the move_do_loop_default function
  Tile::Type ch{Game::level->get_tile(nh)};
  if (!Game::level->is_real(nh) && ch == Tile::Floor &&
      !player->is_levitating()) {
    ch = Tile::Trap;
    Game::level->set_tile(nh, Tile::Trap);
    Game::level->set_real(nh);
  }

  switch (ch) {
    case Tile::Wall:
    case Tile::ClosedDoor: {
      player->set_not_running();
      return false;
    }

    case Tile::Shop: {  // Enter shop
      if (Game::level->shop == nullptr) {
        Game::level->shop = new Shop();
      }
      Game::level->shop->enter();
      player->set_not_running();
    }
      return false;

    case Tile::Floor:
    case Tile::StairsDown:
    case Tile::StairsUp:
    case Tile::Trap:
    case Tile::OpenDoor: return move_do_loop_default(nh);
  }
}

bool move_do(char ch, bool cautiously) {
  int dy{0};
  int dx{0};

  ch = static_cast<char>(tolower(ch));
  switch (ch) {
    case 'h':
      dy = 0;
      dx = -1;
      break;
    case 'j':
      dy = 1;
      dx = 0;
      break;
    case 'k':
      dy = -1;
      dx = 0;
      break;
    case 'l': dy = 0, dx = 1; break;
    case 'y': dy = -1, dx = -1; break;
    case 'u': dy = -1, dx = 1; break;
    case 'b': dy = 1, dx = -1; break;
    case 'n': dy = 1, dx = 1; break;
  }

  // If we cannot really move, return
  if (player_turns_without_moving) {
    player_turns_without_moving--;
    Game::io->message("you are still stuck in the bear trap");
    return true;
  }

  // If we are confused, we don't decide ourselves where to stumble
  if (player->is_confused() && os_rand_range(5) != 0) {
    player->set_not_running();
    to_death = false;

    Coordinate nh{player->possible_random_move()};
    dx = nh.x - player->get_position().x;
    dy = nh.y - player->get_position().y;
  }
  return move_do_loop(dx, dy, cautiously);
}
