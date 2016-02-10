#include <string>

using namespace std;

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

// Find the spot for the chaser(er) to move closer to the chasee(ee).
static Coordinate chase(Monster& monster, Coordinate const& target) {

  // If the thing is confused, let it move randomly.
  // * Invisible Stalkers are slightly confused all of the time
  // * Bats are quite confused all the time
  if ((monster.is_confused() && os_rand_range(5) != 0)
      || (monster.get_type() == 'P' && os_rand_range(5) == 0)
      || (monster.get_type() == 'B' && os_rand_range(2) == 0)) {

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
  for (xy.x = max(mon_pos.x - 1, 0); xy.x <= min(mon_pos.x + 1, NUMCOLS -1); xy.x++) {
    for (xy.y = max(mon_pos.y - 1, 0); xy.y <= min(mon_pos.y + 1, NUMLINES - 2); xy.y++) {

      if (Game::level->can_step(xy.x, xy.y)) {

        // Cannot walk on a scare monster scroll
        Item* xy_item = Game::level->get_item(xy.x, xy.y);
        if (xy_item != nullptr && xy_item->o_type == IO::Scroll &&
            xy_item->o_which == Scroll::SCARE) {
          continue;
        }

        // It can also be a Xeroc, which we shouldn't step on
        Monster* obj = Game::level->get_monster(xy.x, xy.y);
        if (obj != nullptr && obj->get_type() == 'X') {
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

  // If gold has been taken, run after hero
  room* chaser_room = Game::level->get_room(monster->get_position());
  if (monster->is_greedy() && chaser_room != nullptr && chaser_room->r_goldval == 0) {
    monster->t_dest = &player->get_position();
  }

  Coordinate target = *monster->t_dest;
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
    } else if (target == *monster->t_dest) {
      for (Item *obj : Game::level->items) {
        if (monster->t_dest == &obj->get_position()) {
          Game::level->items.remove(obj);
          monster->t_pack.push_back(obj);
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

  // Show movement
  if (chase_coord != monster->get_position()) {
    Tile::Type ch = Game::level->get_tile(chase_coord);
    Tile::Type prev_ch = Game::level->get_tile(monster->get_position());

    // Remove monster from old position IFF we see it, or it was standing on a
    // passage we have previously seen
    Game::level->set_monster(monster->get_position(), nullptr);
    if (((prev_ch == Tile::Floor || prev_ch == Tile::OpenDoor) &&
         Game::level->is_discovered(monster->get_position()))) {
      Game::io->print_tile(monster->get_position());
    }

    // Check if we stepped in a trap
    if ((ch == Tile::Trap || (!Game::level->is_real(chase_coord) && ch == Tile::Floor)) &&
          !monster->is_levitating()) {
      Coordinate orig_pos = monster->get_position();

      Trap::spring(&monster, chase_coord);

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
    monster->set_room(Game::level->get_room(chase_coord));
    monster->set_position(chase_coord);
    Game::level->set_monster(chase_coord, monster);
  }

  return 0;
}

bool
monster_take_turn(Monster* monster)
{
  if (monster == nullptr) {
    error("null parameter");
  }

  // Return if stuck
  if (monster->is_held()) {
    return true;

  // Chase player, if there's a target
  } else if (monster->is_chasing() && monster->t_dest != nullptr) {
    return chase_do(monster) != -1;

  // Chase gold, if greedy
  } else if (!monster->is_chasing() && monster->is_greedy()) {
    room* mon_room = monster->get_room();
    if (mon_room != nullptr) {
      monster->set_target(mon_room->r_goldval
          ? &mon_room->r_gold
          : &player->get_position());
      monster->set_chasing();
    }
    return true;

  // Do nothing
  } else {
    return true;
  }
}

