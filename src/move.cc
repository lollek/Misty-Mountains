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
      || (Game::level->get_flags(x, y) & (F_REAL|F_PASS)) == (F_REAL|F_PASS));
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


/* TODO: Clean up this monster */
bool
move_do(char ch)
{
    bool after = true;
    int dy = 0, dx = 0;
    char fl;
    Coordinate nh;

    switch (ch)
    {
	case 'h': dy =  0; dx = -1; break;
	case 'j': dy =  1; dx =  0; break;
	case 'k': dy = -1; dx =  0; break;
	case 'l': dy =  0, dx =  1; break;
	case 'y': dy = -1, dx = -1; break;
	case 'u': dy = -1, dx =  1; break;
	case 'b': dy =  1, dx = -1; break;
	case 'n': dy =  1, dx =  1; break;
    }

    firstmove = false;
    if (player_turns_without_moving)
    {
	player_turns_without_moving--;
	io_msg("you are still stuck in the bear trap");
	return true;
    }

    if (player_is_confused() && os_rand_range(5) != 0)
    {
	move_random(player, &nh);
	if (nh == *player_get_pos())
	{
	    running = false;
	    to_death = false;
	    return false;
	}
    }
    else
    {
over:
      {
	Coordinate *player_pos = player_get_pos();
	nh.y = player_pos->y + dy;
	nh.x = player_pos->x + dx;
      }
    }

    /* Check if he tried to move off the screen or make an illegal
     * diagonal move, and stop him if he did. */
    if (nh.x < 0 || nh.x >= NUMCOLS || nh.y <= 0 || nh.y >= NUMLINES - 1)
	goto hit_bound;

    if (!diag_ok(player_get_pos(), &nh))
    {
	running = false;
	return false;
    }

    if (running && (*player_get_pos() == nh))
	after = running = false;

    fl = Game::level->get_flags(nh);
    ch = Game::level->get_type(nh);

    if (!(fl & F_REAL) && ch == FLOOR)
    {
	if (!player_is_levitating())
	{
            ch = TRAP;
            Game::level->set_ch(nh, ch);
            int flags = Game::level->get_flags(nh);
            flags |= F_REAL;
            Game::level->set_flags(nh, static_cast<char>(flags));
	}
    }
    else if (player_is_held() && ch != 'F')
    {
      io_msg("you are being held");
      return after;
    }

    switch (ch)
    {
	case SHADOW: case VWALL: case HWALL:
hit_bound:
	    if (passgo && running && (player_get_room()->r_flags & ISGONE)
		&& !player_is_blind())
	    {
		Coordinate *player_pos = player_get_pos();
		bool b1, b2;

		switch (runch)
		{
		    case 'h':
		    case 'l':
			b1 = (player_pos->y != 1 && move_turn_ok(player_pos->y - 1, player_pos->x));
			b2 = (player_pos->y != NUMLINES - 2 && move_turn_ok(player_pos->y + 1, player_pos->x));
			if (!(b1 ^ b2))
			    break;
			if (b1)
			{
			    runch = 'k';
			    dy = -1;
			}
			else
			{
			    runch = 'j';
			    dy = 1;
			}
			dx = 0;
			move_turnref();
			goto over;
		    case 'j':
		    case 'k':
			b1 = (player_pos->x != 0 && move_turn_ok(player_pos->y, player_pos->x - 1));
			b2 = (player_pos->x != NUMCOLS - 1 && move_turn_ok(player_pos->y, player_pos->x + 1));
			if (!(b1 ^ b2))
			    break;
			if (b1)
			{
			    runch = 'h';
			    dx = -1;
			}
			else
			{
			    runch = 'l';
			    dx = 1;
			}
			dy = 0;
			move_turnref();
			goto over;
		}
	    }
	    running = false;
	    after = false;
            break;
	case DOOR:
          {
            Coordinate *player_pos = player_get_pos();
	    running = false;
	    if (Game::level->get_flags(*player_pos) & F_PASS)
		room_enter(&nh);
	    mvaddcch(player_pos->y, player_pos->x, static_cast<chtype>(floor_at()));
	    if ((fl & F_PASS)
                && Game::level->get_ch(move_pos_prev) == DOOR)
		room_leave(&nh);
            player_set_pos(&nh);
          }
            break;
	case TRAP:
          {
            Coordinate *player_pos = player_get_pos();
	    ch = trap_spring(nullptr, &nh);
	    if (ch == T_DOOR || ch == T_TELEP)
		return after;
	    mvaddcch(player_pos->y, player_pos->x, static_cast<chtype>(floor_at()));
	    if ((fl & F_PASS)
                && Game::level->get_ch(move_pos_prev) == DOOR)
		room_leave(&nh);
            player_set_pos(&nh);
          }
          break;
	case PASSAGE:
          {
	    /*
	     * when you're in a corridor, you don't know if you're in
	     * a maze room or not, and there ain't no way to find out
	     * if you're leaving a maze room, so it is necessary to
	     * always recalculate proom.
	     */
            Coordinate *player_pos = player_get_pos();
	    player_set_room(Game::level->get_room(*player_pos));
	    mvaddcch(player_pos->y, player_pos->x, static_cast<chtype>(floor_at()));
	    if ((fl & F_PASS)
                && Game::level->get_ch(move_pos_prev) == DOOR)
		room_leave(&nh);
            player_set_pos(&nh);
          }
          break;
	case FLOOR:
          {
            Coordinate *player_pos = player_get_pos();
	    if (!(fl & F_REAL))
		trap_spring(nullptr, &nh);
		mvaddcch(player_pos->y, player_pos->x, static_cast<chtype>(floor_at()));
		if ((fl & F_PASS)
                    && Game::level->get_ch(move_pos_prev) == DOOR)
		    room_leave(&nh);
                player_set_pos(&nh);
          }
          break;
	default:
	    running = false;
	    if (isupper(ch) || Game::level->get_monster(nh))
		fight_against_monster(&nh, pack_equipped_item(EQUIPMENT_RHAND), false);
	    else
	    {
              Coordinate *player_pos = player_get_pos();
              mvaddcch(player_pos->y, player_pos->x, static_cast<chtype>(floor_at()));
              if ((fl & F_PASS)
                  && Game::level->get_ch(move_pos_prev) == DOOR)
                room_leave(&nh);
              player_set_pos(&nh);
              if (ch != STAIRS) {
                Item *item = Game::level->get_item(nh);
                if (item == nullptr) {
                  error("No item at position");
                }
                pack_pick_up(item, false);
              }
	    }
            break;
    }
    return after;
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

