
#include "string.h"

#include "io.h"
#include "command.h"
#include "pack.h"
#include "daemons.h"
#include "monster.h"
#include "rings.h"
#include "rooms.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "rogue.h"

#include "status_effects.h"

void
fall_asleep(void)
{
  no_command += SLEEPTIME;
  player_stop_running();
  msg("you fall asleep");
}

void
become_stuck(void)
{
  no_move += STUCKTIME;
  player_stop_running();
}

void
become_restored(void)
{
  if (player_strength_is_weakened())
  {
    player_restore_strength();
    msg("you feel your strength returning");
  }
  else
    msg("you feel warm all over");
}

void
become_poisoned(void)
{
  if (player_has_ring_with_ability(R_SUSTSTR))
    msg("you feel momentarily sick");
  else
  {
    player_modify_strength(-(rnd(3) + 1));
    msg("you feel very sick now");
    player_remove_hallucinating();
  }
}

void
become_healed(void)
{
  player_restore_health(roll(player_get_level(), 4), true);
  player_remove_blind();
  msg("you begin to feel better");
}

void
become_extra_healed(void)
{
  player_restore_health(roll(player_get_level(), 8), true);
  player_remove_blind();
  player_remove_hallucinating();
  msg("you begin to feel much better");
}

void
become_stronger(void)
{
  player_modify_strength(1);
  msg("you feel stronger, now.  What bulging muscles!");
}

void
raise_level(void)
{
  player_raise_level();
  if (game_type != QUICK)
    msg("you suddenly feel much more skillful");
}

void
teleport(THING *thing, coord *target)
{
  coord new_pos;

  /* Set target location */
  if (target == NULL)
    do
      room_find_floor(NULL, &new_pos, false, true);
    while (same_coords(new_pos, *player_get_pos()));
  else
  {
    new_pos.y = target->y;
    new_pos.x = target->x;
  }

  /* Move target */
  if (is_player(thing))
  {
    mvaddcch(thing->t_pos.y, thing->t_pos.x, floor_at());
    if (roomin(&new_pos) != player_get_room())
    {
      room_leave(player_get_pos());
      player_set_pos(&new_pos);
      room_enter(player_get_pos());
    }
    else
    {
      player_set_pos(&new_pos);
      look(true);
    }
  }
  else if (same_coords(new_pos, thing->t_pos) && see_monst(thing))
    msg("%s looks confused for a moment", set_mname(thing));
  else
  {
    if (see_monst(thing))
    {
      mvaddcch(thing->t_pos.y, thing->t_pos.x, thing->t_oldch);
      msg(cansee(new_pos.y, new_pos.x)
            ? "%s teleported a short distance"
            : "%s suddenly disappeared", set_mname(thing));
    }
    else if (cansee(new_pos.y, new_pos.x))
      msg("%s appeared from thin air", set_mname(thing));

    set_oldch(thing, &new_pos);
    moat(thing->t_pos.y, thing->t_pos.x) = NULL;
    if (roomin(&new_pos) != thing->t_room)
    {
      thing->t_dest = monster_destination(thing);
      thing->t_room = roomin(&new_pos);
    }
    thing->t_pos = new_pos;
    moat(new_pos.y, new_pos.x) = thing;
  }

  /* Print @ new location */
  if (is_player(thing))
  {
    mvaddcch(new_pos.y, new_pos.x, PLAYER);
    if (player_is_held())
    {
      player_remove_held();
      vf_hit = 0;
    }
    no_move = 0;
    command_stop(true);
    flushinp();
    msg("suddenly you're somewhere else");
  }
  else if (see_monst(thing))
    mvaddcch(new_pos.y, new_pos.x, thing->t_disguise);
  else if (player_can_sense_monsters())
    mvaddcch(new_pos.y, new_pos.x, thing->t_type | A_STANDOUT);

}
