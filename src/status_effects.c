
#include "string.h"

#include "rogue.h"
#include "io.h"
#include "command.h"
#include "pack.h"
#include "daemons.h"
#include "monsters.h"

#include "status_effects.h"

void
set_true_seeing(THING *thing, bool status, bool permanent)
{
  if (thing != &player)
  {
    set_status(thing, status, CANSEE);
    return;
  }

  /* Add effect */
  if (status)
  {
    if (is_true_seeing(&player))
      daemon_lengthen_fuse(daemon_remove_true_seeing, SEEDURATION);
    else
    {
      player.t_flags |= CANSEE;
      if (!permanent)
        daemon_start_fuse(daemon_remove_true_seeing, 0, SEEDURATION, AFTER);
      look(false);
    }
    msg("everything suddenly looks sharper");
    cure_blindness();
  }

  /* Remove effect */
  else
  {
    THING *th;
    for (th = mlist; th != NULL; th = th->l_next)
      if (is_invisible(th) && see_monst(th))
        mvaddcch(th->t_pos.y, th->t_pos.x, th->t_oldch);
    player.t_flags &= ~CANSEE;
  }
}

void
daemon_remove_true_seeing(void)
{
  set_true_seeing(&player, false, false);
}

void
fall_asleep(void)
{
  no_command += SLEEPTIME;
  player.t_flags &= ~ISRUN;
  msg("you fall asleep");
}

void
become_stuck(void)
{
  no_move += STUCKTIME;
  player.t_flags &= ~ISRUN;
}

void
become_restored(void)
{
  if (pstats.s_str < max_stats.s_str)
  {
    int i;
    for (i = 0; i < RING_SLOTS_SIZE; ++i)
    {
      THING *ring = pack_equipped_item(ring_slots[i]);
      if (ring != NULL && ring->o_which == R_ADDSTR)
        add_str(&pstats.s_str, -ring->o_arm);
    }

    pstats.s_str = max_stats.s_str;

    for (i = 0; i < RING_SLOTS_SIZE; ++i)
    {
      THING *ring = pack_equipped_item(ring_slots[i]);
      if (ring != NULL && ring->o_which == R_ADDSTR)
        add_str(&pstats.s_str, ring->o_arm);
    }

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
    chg_str(-(rnd(3) + 1));
    msg("you feel very sick now");
    remove_tripping();
  }
}

void
become_confused(bool permanent)
{
  if (is_confused(&player))
    daemon_lengthen_fuse(remove_confusion, HUHDURATION);
  else
  {
    set_confused(&player, true);
    if (!permanent)
      daemon_start_fuse(remove_confusion, 0, HUHDURATION, AFTER);
    look(false);
  }
  msg(is_hallucinating(&player)
    ? "what a trippy feeling!"
    : "wait, what's going on here. Huh? What? Who?");
}

void
remove_confusion(void)
{
  set_confused(&player, false);
  msg("you feel less %s now",
    is_hallucinating(&player) ? "trippy" : "confused");
}

void
become_healed(void)
{
  if ((pstats.s_hpt += roll(pstats.s_lvl, 4)) > max_hp)
    pstats.s_hpt = ++max_hp;
  cure_blindness();
  msg("you begin to feel better");
}

void
become_extra_healed(void)
{
  if ((pstats.s_hpt += roll(pstats.s_lvl, 8)) > max_hp)
  {
    if (pstats.s_hpt > max_hp + pstats.s_lvl + 1)
      ++max_hp;
    pstats.s_hpt = ++max_hp;
  }
  cure_blindness();
  remove_tripping();
  msg("you begin to feel much better");
}

void
become_stronger(void)
{
  chg_str(1);
  msg("you feel stronger, now.  What bulging muscles!");
}

void
become_monster_seeing(bool permanent)
{
  player.t_flags |= SEEMONST;
  if (!permanent)
    daemon_start_fuse((void(*)())turn_see, true, MFINDDURATION, AFTER);
  /* FIXME: Make sure that this work */
  if (!turn_see(false))
    msg("you have a %s feeling for a moment, then it passes",
        is_hallucinating(&player) ? "normal" : "strange");
}

void
become_tripping(bool permanent)
{
  if (is_hallucinating(&player))
    daemon_lengthen_fuse(remove_tripping, SEEDURATION);
  else
  {
    if (on(player, SEEMONST))
      turn_see(false);
    daemon_start(daemon_change_visuals, 0, BEFORE);
    set_hallucinating(&player, true);
    if (!permanent)
      daemon_start_fuse(remove_tripping, 0, SEEDURATION, AFTER);
    look(false);
  }
  msg("Oh, wow!  Everything seems so cosmic!");
}

void
remove_tripping(void)
{
  THING *tp;

  if (!is_hallucinating(&player))
    return;

  daemon_kill(daemon_change_visuals);
  set_hallucinating(&player, false);

  if (is_blind(&player))
    return;

  /* undo the things */
  for (tp = lvl_obj; tp != NULL; tp = tp->l_next)
    if (cansee(tp->o_pos.y, tp->o_pos.x))
      mvaddcch(tp->o_pos.y, tp->o_pos.x, (chtype)tp->o_type);

  /* undo the monsters */
  for (tp = mlist; tp != NULL; tp = tp->l_next)
  {
    move(tp->t_pos.y, tp->t_pos.x);
    if (cansee(tp->t_pos.y, tp->t_pos.x))
      if (!is_invisible(tp) || is_true_seeing(&player))
        addcch(tp->t_disguise);
      else
        addcch(chat(tp->t_pos.y, tp->t_pos.x));
    else if (on(player, SEEMONST))
      addcch(tp->t_type | A_STANDOUT);
  }
  msg("Everything looks SO boring now.");
}

void
become_hasted(bool permanent)
{
  after = false;
  if (on(player, ISHASTE))
  {
    no_command += rnd(8);
    player.t_flags &= ~(ISRUN|ISHASTE);
    daemon_extinguish_fuse(remove_hasted);
    msg("you faint from exhaustion");
    return;
  }
  else
  {
    player.t_flags |= ISHASTE;
    if (!permanent)
      daemon_start_fuse(remove_hasted, 0, HASTEDURATION, AFTER);
    msg("you feel yourself moving much faster");
  }
}

void
remove_hasted(void)
{
  player.t_flags &= ~ISHASTE;
  msg("you feel yourself slowing down");
}

void become_blind(bool permanent)
{
  if (is_blind(&player))
    daemon_lengthen_fuse(cure_blindness, SEEDURATION);
  else
  {
    set_blind(&player, true);
    if (!permanent)
      daemon_start_fuse(cure_blindness, 0, SEEDURATION, AFTER);
    look(false);
  }
  msg(is_hallucinating(&player)
    ? "oh, bummer!  Everything is dark!  Help!"
    : "a cloak of darkness falls around you");
}

void
cure_blindness(void)
{
  if (is_blind(&player))
  {
    daemon_extinguish_fuse(cure_blindness);
    set_blind(&player, false);
    if (!(proom->r_flags & ISGONE))
      enter_room(&hero);
    msg(is_hallucinating(&player)
      ? "far out!  Everything is all cosmic again"
      : "the veil of darkness lifts");
  }
}

void
become_levitating(bool permanent)
{
  if (is_levitating(&player))
    daemon_lengthen_fuse(remove_levitating, LEVITDUR);
  else
  {
    set_levitating(&player, true);
    if (!permanent)
      daemon_start_fuse(remove_levitating, 0, LEVITDUR, AFTER);
    look(false);
  }
  msg(is_hallucinating(&player)
    ? "oh, wow!  You're floating in the air!"
    : "you start to float in the air");
}

void
remove_levitating(void)
{
  set_levitating(&player, false);
  msg(is_hallucinating(&player)
    ? "bummer!  You've hit the ground"
    : "you float gently to the ground");
}

void
raise_level(void)
{
  pstats.s_exp = e_levels[pstats.s_lvl-1] + 1L;
  check_level();
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
      find_floor(NULL, &new_pos, false, true);
    while (same_coords(new_pos, hero));
  else
  {
    new_pos.y = target->y;
    new_pos.x = target->x;
  }

  /* Move target */
  if (thing == &player)
  {
    mvaddcch(thing->t_pos.y, thing->t_pos.x, floor_at());
    if (roomin(&new_pos) != proom)
    {
      leave_room(&hero);
      hero = new_pos;
      enter_room(&hero);
    }
    else
    {
      hero = new_pos;
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
  if (thing == &player)
  {
    mvaddcch(new_pos.y, new_pos.x, PLAYER);
    if (on(player, ISHELD))
    {
      player.t_flags &= ~ISHELD;
      vf_hit = 0;
    }
    no_move = 0;
    command_stop(true);
    flushinp();
    msg("suddenly you're somewhere else");
  }
  else if (see_monst(thing))
    mvaddcch(new_pos.y, new_pos.x, thing->t_disguise);
  else if (on(player, SEEMONST))
    mvaddcch(new_pos.y, new_pos.x, thing->t_type | A_STANDOUT);

}
