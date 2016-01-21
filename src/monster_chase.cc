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

#define DRAGONSHOT  5  /* one chance in DRAGONSHOT that a dragon will flame */
static Coordinate ch_ret;   /* Where chasing takes you */

static bool
chase_as_confused(Monster *tp, Coordinate *ee)
{
  int curdist;

  /* get a valid random move */
  ch_ret = tp->possible_random_move();
  curdist = dist_cp(&ch_ret, ee);

  /* Small chance that it will become un-confused */
  if (os_rand_range(20) == 0)
    tp->set_not_confused();

  return curdist != 0 && !(ch_ret == player->get_position());
}


/** Chase
 * Find the spot for the chaser(er) to move closer to the chasee(ee). 
 * Returns true if we want to keep on chasing later 
 * TODO: Clean up this monster */
static bool
chase(Monster *tp, Coordinate *ee)
{
  int x;
  int y;
  int curdist;
  int thisdist;
  Coordinate const& er = tp->get_position();
  char ch;
  int plcnt = 1;
  static Coordinate tryp;
  int ey;
  int ex;

  /* If the thing is confused, let it move randomly. Invisible
   * Stalkers are slightly confused all of the time, and bats are
   * quite confused all the time */
  if ((tp->is_confused() && os_rand_range(5) != 0)
      || (tp->get_type() == 'P' && os_rand_range(5) == 0)
      || (tp->get_type() == 'B' && os_rand_range(2) == 0))
    return chase_as_confused(tp, ee);



  /* Otherwise, find the empty spot next to the chaser that is
   * closest to the chasee. This will eventually hold where we
   * move to get closer. If we can't find an empty spot,
   * we stay where we are */
  curdist = dist_cp(&er, ee);
  ch_ret = er;

  ey = er.y + 1;
  if (ey >= NUMLINES - 1)
    ey = NUMLINES - 2;

  ex = er.x + 1;
  if (ex >= NUMCOLS)
    ex = NUMCOLS - 1;

  for (x = er.x - 1; x <= ex; x++)
  {
    if (x < 0)
      continue;

    tryp.x = x;
    for (y = er.y - 1; y <= ey; y++)
    {
      tryp.y = y;

      if (!diag_ok(&er, &tryp))
        continue;

      ch = Game::level->get_type(x, y);
      if (step_ok(ch))
      {
        /* If it is a scroll, it might be a scare monster scroll
         * so we need to look it up to see what type it is */
        if (ch == SCROLL)
        {
          Item* obj = Game::level->get_item(x, y);
          if (obj != nullptr && obj->o_which == S_SCARE) {
            continue;
          }
        }
        /* It can also be a Xeroc, which we shouldn't step on */
        Monster* obj = Game::level->get_monster(x, y);
        if (obj != nullptr && obj->get_type() == 'X')
          continue;

        /* If we didn't find any scrolls at this place or it
         * wasn't a scare scroll, then this place counts */
        thisdist = dist(y, x, ee->y, ee->x);
        if (thisdist < curdist)
        {
          plcnt = 1;
          ch_ret = tryp;
          curdist = thisdist;
        }
        else if (thisdist == curdist && os_rand_range(++plcnt) == 0)
        {
          ch_ret = tryp;
          curdist = thisdist;
        }
      }
    }
  }
  return curdist != 0 && !(ch_ret == player->get_position());
}


/* TODO: Clean up this monster */
static int
chase_do(Monster *th)
{
    Coordinate *cp;
    room *rer; /* room of chaser, */
    room *ree; /* room of chasee */
    int mindist = 32767;
    int curdist;
    bool stoprun = false; /* true means we are there */
    bool door;
    static Coordinate m_this; /* Temporary destination for chaser */
    Coordinate delta;

    rer = th->get_room();

    if (th->is_greedy() && rer->r_goldval == 0)
	th->t_dest = &player->get_position();	/* If gold has been taken, run after hero */

    if (th->t_dest == nullptr) {
      error("Cannot chase after null");
    }

    if (th->t_dest == &player->get_position())	/* Find room of chasee */
	ree = player->get_room();
    else
	ree = Game::level->get_room(*th->t_dest);

    /* We don't count doors as inside rooms for this routine */
    door = (Game::level->get_ch(th->get_position()) == DOOR);

    /* If the object of our desire is in a different room,
     * and we are not in a corridor, run to the door nearest to
     * our goal */
over:
    if (rer != ree)
    {
	for (cp = rer->r_exit; cp < &rer->r_exit[rer->r_nexits]; cp++)
	{
	    curdist = dist_cp(th->t_dest, cp);
	    if (curdist < mindist)
	    {
		m_this = *cp;
		mindist = curdist;
	    }
	}
	if (door)
	{
	    rer = Game::level->get_passage(th->get_position());
	    door = false;
	    goto over;
	}
    }
    else
    {
	m_this = *th->t_dest;
	
	/* For dragons check and see if (a) the hero is on a straight
	 * line from it, and (b) that it is within shooting distance,
	 * but outside of striking range */
	if (th->get_type() == 'D' && (th->get_position().y == player->get_position().y
              || th->get_position().x == player->get_position().x
              || abs(th->get_position().y - player->get_position().y)
                  == abs(th->get_position().x - player->get_position().x))
	    && dist_cp(&th->get_position(), &player->get_position()) <= BOLT_LENGTH * BOLT_LENGTH
	    && !th->is_cancelled() && os_rand_range(DRAGONSHOT) == 0)
	{
	    delta.y = sign(player->get_position().y - th->get_position().y);
	    delta.x = sign(player->get_position().x - th->get_position().x);
            Coordinate position = th->get_position();
	    magic_bolt(&position, &delta, "flame");
	    command_stop(true);
	    daemon_reset_doctor();
	    if (to_death && !th->is_players_target())
              to_death = false;
	    return(0);
	}
    }

    /*
     * This now contains what we want to run to this time
     * so we run to it.  If we hit it we either want to fight it
     * or stop running
     */
    if (!chase(th, &m_this))
    {
	if (m_this == player->get_position())
	    return( fight_against_player(th) );
	else if (m_this == *th->t_dest)
	{
            for (Item *obj : Game::level->items)
		if (th->t_dest == &obj->get_pos())
		{
                    Game::level->items.remove(obj);
                    th->t_pack.push_back(obj);
                    Game::level->set_ch(obj->get_pos(),
			(th->get_room()->r_flags & ISGONE) ? PASSAGE : FLOOR);
		    monster_find_new_target(th);
		    break;
		}
	    if (th->get_type() != 'F')
		stoprun = true;
	}
    }
    else
    {
	if (th->get_type() == 'F')
	    return(0);
    }

    if (th->is_stuck())
      return 1;

    if (!(ch_ret == th->get_position()))
    {
      char ch = Game::level->get_type(ch_ret);

      /* Remove monster from old position */
      mvaddcch(th->get_position().y, th->get_position().x,
               static_cast<chtype>(th->t_oldch));
      Game::level->set_monster(th->get_position(), nullptr);

      /* Check if we stepped in a trap */
      if (ch == TRAP || (!Game::level->is_real(ch_ret) && ch == FLOOR))
      {
        Coordinate orig_pos = th->get_position();

        trap_spring(th, &ch_ret);
        if (monster_is_dead(th))
          return -1;

        /* If we've been mysteriously misplaced, let's not touch anything */
        if (!(orig_pos == th->get_position()))
          return 0;
      }

      /* Put monster in new position */
      th->set_oldch(ch_ret);
      th->set_room(Game::level->get_room(ch_ret));
      th->set_position(ch_ret);
      Game::level->set_monster(ch_ret, th);
    }
    move(ch_ret.y, ch_ret.x);

    if (monster_seen_by_player(th))
      addcch(static_cast<chtype>(th->t_disguise));
    else if (player->can_sense_monsters())
      addcch(static_cast<chtype>(th->get_type())| A_STANDOUT);

    /* And stop running if need be */
    if (stoprun && (&th->get_position() == th->t_dest))
      th->set_not_running();

    return(0);
}

bool
monster_chase(Monster *tp)
{
  if (!tp->is_slowed() || tp->t_turn)
    if (chase_do(tp) == -1)
      return false;

  if (tp->is_hasted())
    if (chase_do(tp) == -1)
      return false;

  tp->t_turn ^= true;
  return true;
}

