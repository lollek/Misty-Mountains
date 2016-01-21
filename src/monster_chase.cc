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
#include "monster_private.h"

static bool chase_as_confused(Monster& monster, Coordinate const& coord,
                              Coordinate& retval) {
  // get a valid random move
  retval = monster.possible_random_move();
  int curdist = dist_cp(&retval, &coord);

  // Small chance that it will become un-confused
  if (os_rand_range(20) == 0) {
    monster.set_not_confused();
  }

  return curdist != 0 && !(retval == player->get_position());
}


// Find the spot for the chaser(er) to move closer to the chasee(ee).
// Returns true if we want to keep on chasing later
// TODO: Clean up this monster
static bool chase(Monster& monster, Coordinate const& coord, Coordinate& retval) {


  // If the thing is confused, let it move randomly.
  // Invisible Stalkers are slightly confused all of the time
  // Bats are quite confused all the time
  if ((monster.is_confused() && os_rand_range(5) != 0)
      || (monster.get_type() == 'P' && os_rand_range(5) == 0)
      || (monster.get_type() == 'B' && os_rand_range(2) == 0))
    return chase_as_confused(monster, coord, retval);

  // Otherwise, find the empty spot next to the chaser that is
  // closest to the chasee. This will eventually hold where we
  // move to get closer. If we can't find an empty spot,
  // we stay where we are
  Coordinate const& er = monster.get_position();
  int curdist = dist_cp(&er, &coord);
  retval = er;

  int plcnt = 1;
  Coordinate xy;
  for (xy.x = max(er.x - 1, 0); xy.x <= min(er.x + 1, NUMCOLS -1); xy.x++) {
    for (xy.y = max(er.y - 1, 0); xy.y <= min(er.y + 1, NUMLINES - 2); xy.y++) {

      // If we cannot move there, skip it;
      if (!diag_ok(&er, &xy)) {
        continue;
      }

      char ch = Game::level->get_type(xy.x, xy.y);
      if (step_ok(ch)) {

        // Cannot walk on a scare monster scroll
        if (ch == SCROLL) {
          Item* obj = Game::level->get_item(xy.x, xy.y);
          if (obj != nullptr && obj->o_which == S_SCARE) {
            continue;
          }
        }

        // It can also be a Xeroc, which we shouldn't step on
        Monster* obj = Game::level->get_monster(xy.x, xy.y);
        if (obj != nullptr && obj->get_type() == 'X') {
          continue;
        }

        int thisdist = dist_cp(&xy, &coord);
        if (thisdist < curdist) {
          plcnt = 1;
          retval = xy;
          curdist = thisdist;
        }

        else if (thisdist == curdist && os_rand_range(++plcnt) == 0) {
          retval = xy;
          curdist = thisdist;
        }
      }
    }
  }
  return curdist != 0 && !(retval == player->get_position());
}


// TODO: Clean up this monster
static int
chase_do(Monster& monster)
{
  if (monster.t_dest == nullptr) {
    error("Cannot chase after null");
  }

  Coordinate m_this; /* Temporary destination for chaser */

  // Room of chaser
  room* rer = monster.get_room();

  // If gold has been taken, run after hero
  if (monster.is_greedy() && rer->r_goldval == 0) {
    monster.t_dest = &player->get_position();
  }

  // Find room of chasee
  room* ree = monster.t_dest == &player->get_position()
    ? player->get_room()
    : Game::level->get_room(*monster.t_dest);

  // We don't count doors as inside rooms for this routine
  bool door = Game::level->get_ch(monster.get_position()) == DOOR;

  // If the object of our desire is in a different room,
  // and we are not in a corridor, run to the door nearest to
  // our goal
over:
  if (rer != ree) {
    for (int i = 0, mindist = 32767; i < rer->r_nexits; ++i) {
      int curdist = dist_cp(monster.t_dest, &rer->r_exit[i]);
      if (curdist < mindist) {
        m_this = rer->r_exit[i];
        mindist = curdist;
      }
    }

    if (door) {
      rer = Game::level->get_passage(monster.get_position());
      door = false;
      goto over;
    }

  } else {
    m_this = *monster.t_dest;
    if (monster_try_breathe_fire_on_player(monster)) {
      return 0;
    }
  }

   // This now contains what we want to run to this time
   // so we run to it.  If we hit it we either want to fight it
   // or stop running
  bool stoprun = false; // true means we are there
  Coordinate chase_coord;
  if (!chase(monster, m_this, chase_coord)) {

    // If we have run into the player, fight it
    if (m_this == player->get_position()) {
      return fight_against_player(&monster);

    // If we have run into something else we like, pick it up
    } else if (m_this == *monster.t_dest) {
      for (Item *obj : Game::level->items) {
        if (monster.t_dest == &obj->get_pos()) {
          Game::level->items.remove(obj);
          monster.t_pack.push_back(obj);
          Game::level->set_ch(obj->get_pos(),
              (monster.get_room()->r_flags & ISGONE) ? PASSAGE : FLOOR);
          monster_find_new_target(&monster);
          break;
        }
      }

      if (monster.get_type() != 'F') {
        stoprun = true;
      }
    }

  } else {
    if (monster.get_type() == 'F') {
      return 0;
    }
  }

  if (monster.is_stuck()) {
    return 1;
  }

  if (chase_coord != monster.get_position()) {
    char ch = Game::level->get_type(chase_coord);

    // Remove monster from old position
    mvaddcch(monster.get_position().y, monster.get_position().x,
        static_cast<chtype>(monster.t_oldch));
    Game::level->set_monster(monster.get_position(), nullptr);

    // Check if we stepped in a trap
    if (ch == TRAP || (!Game::level->is_real(chase_coord) && ch == FLOOR)) {
      Coordinate orig_pos = monster.get_position();

      trap_spring(&monster, &chase_coord);
      if (monster_is_dead(&monster)) {
        return -1;
      }

      // If we've been mysteriously misplaced, let's not touch anything
      if (orig_pos != monster.get_position()) {
        return 0;
      }
    }

    // Put monster in new position
    monster.set_oldch(chase_coord);
    monster.set_room(Game::level->get_room(chase_coord));
    monster.set_position(chase_coord);
    Game::level->set_monster(chase_coord, &monster);
  }

  if (monster_seen_by_player(&monster)) {
    mvaddcch(chase_coord.y, chase_coord.x, static_cast<chtype>(monster.t_disguise));
  } else if (player->can_sense_monsters()) {
    mvaddcch(chase_coord.y, chase_coord.x, static_cast<chtype>(monster.get_type())| A_STANDOUT);
  }

  // And stop running if need be
  if (stoprun && (&monster.get_position() == monster.t_dest)) {
    monster.set_not_running();
  }

  return 0;
}

bool
monster_chase(Monster *tp)
{
  if (tp == nullptr) {
    error("monster_chase for null monster");
  }

  if (!tp->is_slowed() || tp->t_turn)
    if (chase_do(*tp) == -1)
      return false;

  if (tp->is_hasted())
    if (chase_do(*tp) == -1)
      return false;

  tp->t_turn ^= true;
  return true;
}

