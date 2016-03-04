#include <string>

#include "error_handling.h"
#include "game.h"
#include "scrolls.h"
#include "command.h"
#include "io.h"
#include "traps.h"
#include "daemons.h"
#include "fight.h"
#include "move.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "wand.h"
#include "rogue.h"
#include "os.h"
#include "magic.h"

#include "monster.h"

using namespace std;

// Find the spot for the chaser(er) to move closer to the chasee(ee).
static Coordinate chase(Monster& monster, Coordinate const& target) {

  // If the thing is confused, let it move randomly.
  // * Invisible Stalkers are slightly confused all of the time
  // * Bats are quite confused all the time
  if ((monster.is_confused() && os_rand_range(5) != 0)) {

    // Small chance that it will become un-confused
    if (os_rand_range(20) == 0) {
      monster.set_not_confused();
    }
    return monster.possible_random_move();
  }

  // Otherwise, find the empty spot next to the chaser that is
  // closest to the chasee. This will eventually hold where we
  // move to get closer. If we can't find an empty spot,
  // we stay where we are
  Coordinate const& mon_pos = monster.get_position();
  int curdist = dist_cp(&mon_pos, &target);
  Coordinate retval = mon_pos;
  int plcnt = 1;

  Coordinate xy;
  for (xy.x = max(mon_pos.x - 1, 0); xy.x <= min(mon_pos.x + 1, IO::map_width -1); xy.x++) {
    for (xy.y = max(mon_pos.y - 1, 0); xy.y <= min(mon_pos.y + 1, IO::map_height - 1); xy.y++) {

      if (Game::level->can_step(xy.x, xy.y)) {

        // Cannot walk on a scare monster scroll
        Item* xy_item = Game::level->get_item(xy.x, xy.y);
        if (xy_item != nullptr && xy_item->o_type == IO::Scroll &&
            xy_item->o_which == Scroll::SCARE) {
          continue;
        }

        // If we are closer, we pick this as a good position
        int thisdist = dist_cp(&xy, &target);
        if (thisdist < curdist) {
          plcnt = 1;
          retval = xy;
          curdist = thisdist;
        }

        // If it's as close as a previous coordinate, we might pick it
        else if (thisdist == curdist && os_rand_range(++plcnt) == 0) {
          retval = xy;
          curdist = thisdist;
        }
      }
    }
  }
  return retval;
}


// TODO: Clean up this monster
static int
chase_do(Monster* monster)
{
  if (monster == nullptr) {
    error("monster = null");
  }

  Coordinate target = *monster->get_target();
  if (monster_try_breathe_fire_on_player(*monster)) {
    return 0;
  }

  Coordinate chase_coord = chase(*monster, target);

  // If we have reached the target, do stuff
  if (dist_cp(&chase_coord, &target) == 0) {
    // Reached player, and want to fight
    if (chase_coord == player->get_position()) {
      return fight_against_player(monster);

    // Reached shiny thing
    } else if (target == *monster->get_target()) {
      for (Item *obj : Game::level->items) {
        if (monster->get_target() == &obj->get_position()) {
          Game::level->items.remove(obj);
          monster->get_pack().push_back(obj);
          monster->find_new_target();
          monster->set_not_running();
          break;
        }
      }
    }
  }

  if (monster->is_stuck()) {
    return 1;
  }


  // If we moved
  if (chase_coord != monster->get_position()) {

    // Remove monster from old position
    Game::level->set_monster(monster->get_position(), nullptr);

    // Check if we stepped in a trap
    bool was_trap = false;
    Tile::Type ch = Game::level->get_tile(chase_coord);
    if (!Game::level->is_real(chase_coord) && Game::level->get_tile(chase_coord)) {
      was_trap = true;

      // Only show it's a trap if player can see this position
      if (player->can_see(chase_coord)) {
        Game::level->set_real(chase_coord);
        Game::level->set_tile(chase_coord, Tile::Trap);
        Game::level->set_discovered(chase_coord);
      }
    }

    if ((ch == Tile::Trap || was_trap) && !monster->is_levitating()) {
      Coordinate orig_pos = monster->get_position();

      Trap::Type trap = Game::level->get_trap_type(chase_coord);
      if (trap == Trap::NTRAPS) {
        trap = Trap::random();
        Game::level->set_trap_type(chase_coord, trap);
      }
      Trap::spring(&monster, trap);

      // Monster is dead?
      if (monster == nullptr) {
        return -1;
      }

      // If we've been mysteriously misplaced, let's not touch anything
      if (orig_pos != monster->get_position()) {
        return 0;
      }
    }

    // Put monster in new position
    monster->set_position(chase_coord);
    Game::level->set_monster(chase_coord, monster);
  }

  return 0;
}

bool Monster::take_turn()
{
  // Return if stuck
  if (is_held()) {
    return true;


  // Chase player, if there's a target
  } else if (is_chasing() && get_target() != nullptr) {
    return chase_do(this) != -1;


  // If monster sees player and is mean, have a chance to attack
  } else if (is_mean() && player->can_see(*this) && os_rand_range(2)) {
    set_target(&player->get_position());
    set_chasing();
    return chase_do(this) != -1;


  // Do nothing
  } else {
    return true;
  }
}

