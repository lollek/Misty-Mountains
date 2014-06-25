
#include "status_effects.h"
#include "rogue.h"

inline bool
is_status(THING *thing, short flag)
{
  return (thing->t_flags & flag) != false;
}
inline bool is_confusing(THING *thing)     { return is_status(thing, CANHUH);  }
inline bool is_true_seeing(THING *thing)   { return is_status(thing, CANSEE);  }
inline bool is_blind(THING *thing)         { return is_status(thing, ISBLIND); }
inline bool is_cancelled(THING *thing)     { return is_status(thing, ISCANC);  }
inline bool is_levitating(THING *thing)    { return is_status(thing, ISLEVIT); }
inline bool is_confused(THING *thing)      { return is_status(thing, ISHUH);   }
inline bool is_invisible(THING *thing)     { return is_status(thing, ISINVIS); }
inline bool is_hallucinating(THING *thing) { return is_status(thing, ISHALU);  }

inline void
set_status(THING *thing, bool status, short flag)
{
  thing->t_flags = status
    ? thing->t_flags | flag
    : thing->t_flags & ~flag;
}
inline void set_confusing(THING *t, bool b)       { set_status(t, b, CANHUH);  }
inline void set_blind(THING *t, bool b)           { set_status(t, b, ISBLIND); }
inline void set_cancelled(THING *t, bool b)       { set_status(t, b, ISCANC);  }
inline void set_levitating(THING *t, bool b)      { set_status(t, b, ISLEVIT); }
inline void set_confused(THING *t, bool b)        { set_status(t, b, ISHUH);   }
inline void set_invisible(THING *t, bool b)       { set_status(t, b, ISINVIS); }
inline void set_hallucinating(THING *t, bool b)   { set_status(t, b, ISHALU);  }

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
      lengthen(daemon_remove_true_seeing, SEEDURATION);
    else
    {
      player.t_flags |= CANSEE;
      if (!permanent)
        fuse(daemon_remove_true_seeing, 0, SEEDURATION, AFTER);
      look(false);
    }
    msg("everything suddenly looks sharper");
    cure_blindness();
  }

  /* Remove effect */
  else
  {
    THING *th;
    for (th = mlist; th != NULL; th = next(th))
      if (is_invisible(th) && see_monst(th))
        mvaddcch(th->t_pos.y, th->t_pos.x, th->t_oldch);
    player.t_flags &= ~CANSEE;
  }
}

void
daemon_remove_true_seeing()
{
  set_true_seeing(&player, false, false);
}

void
fall_asleep()
{
  no_command += SLEEPTIME;
  player.t_flags &= ~ISRUN;
  msg("you fall asleep");
}

void
become_restored()
{
  if (ISRING(LEFT, R_ADDSTR))
    add_str(&pstats.s_str, -cur_ring[LEFT]->o_arm);
  if (ISRING(RIGHT, R_ADDSTR))
    add_str(&pstats.s_str, -cur_ring[RIGHT]->o_arm);
  if (pstats.s_str < max_stats.s_str)
    pstats.s_str = max_stats.s_str;
  if (ISRING(LEFT, R_ADDSTR))
    add_str(&pstats.s_str, cur_ring[LEFT]->o_arm);
  if (ISRING(RIGHT, R_ADDSTR))
    add_str(&pstats.s_str, cur_ring[RIGHT]->o_arm);
  msg("you feel warm all over");
}

void
become_poisoned()
{
  if (ISWEARING(R_SUSTSTR))
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
    lengthen(remove_confusion, HUHDURATION);
  else
  {
    set_confused(&player, true);
    if (!permanent)
      fuse(remove_confusion, 0, HUHDURATION, AFTER);
    look(false);
  }
  msg(is_hallucinating(&player)
    ? "what a trippy feeling!"
    : "wait, what's going on here. Huh? What? Who?");
}

void
remove_confusion()
{
  set_confused(&player, false);
  msg("you feel less %s now",
    is_hallucinating(&player) ? "trippy" : "confused");
}

void
become_healed()
{
  if ((pstats.s_hpt += roll(pstats.s_lvl, 4)) > max_hp)
    pstats.s_hpt = ++max_hp;
  cure_blindness();
  msg("you begin to feel better");
}

void
become_extra_healed()
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
become_stronger()
{
  chg_str(1);
  msg("you feel stronger, now.  What bulging muscles!");
}

void
become_monster_seeing(bool permanent)
{
  player.t_flags |= SEEMONST;
  if (!permanent)
    fuse((void(*)())turn_see, true, MFINDDURATION, AFTER);
  /* FIXME: Make sure that this work */
  if (!turn_see(false))
    msg("you have a %s feeling for a moment, then it passes",
        is_hallucinating(&player) ? "normal" : "strange");
}

void
become_tripping(bool permanent)
{
  if (is_hallucinating(&player))
    lengthen(remove_tripping, SEEDURATION);
  else
  {
    if (on(player, SEEMONST))
      turn_see(false);
    start_daemon(visuals, 0, BEFORE);
    seenstairs = seen_stairs();
    set_hallucinating(&player, true);
    if (!permanent)
      fuse(remove_tripping, 0, SEEDURATION, AFTER);
    look(false);
  }
  msg("Oh, wow!  Everything seems so cosmic!");
}

void
remove_tripping()
{
  register THING *tp;

  if (!is_hallucinating(&player))
    return;

  kill_daemon(visuals);
  set_hallucinating(&player, false);

  if (is_blind(&player))
    return;

  /* undo the things */
  for (tp = lvl_obj; tp != NULL; tp = next(tp))
    if (cansee(tp->o_pos.y, tp->o_pos.x))
      mvaddcch(tp->o_pos.y, tp->o_pos.x, tp->o_type);

  /* undo the monsters */
  for (tp = mlist; tp != NULL; tp = next(tp))
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
    extinguish(remove_hasted);
    msg("you faint from exhaustion");
    return;
  }
  else
  {
    player.t_flags |= ISHASTE;
    if (!permanent)
      fuse(remove_hasted, 0, HASTEDURATION, AFTER);
    msg("you feel yourself moving much faster");
  }
}

void
remove_hasted()
{
  player.t_flags &= ~ISHASTE;
  msg("you feel yourself slowing down");
}

void become_blind(bool permanent)
{
  if (is_blind(&player))
    lengthen(cure_blindness, SEEDURATION);
  else
  {
    set_blind(&player, true);
    if (!permanent)
      fuse(cure_blindness, 0, SEEDURATION, AFTER);
    look(false);
  }
  msg(is_hallucinating(&player)
    ? "oh, bummer!  Everything is dark!  Help!"
    : "a cloak of darkness falls around you");
}

void
cure_blindness()
{
  if (is_blind(&player))
  {
    extinguish(cure_blindness);
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
    lengthen(remove_levitating, LEVITDUR);
  else
  {
    set_levitating(&player, true);
    if (!permanent)
      fuse(remove_levitating, 0, LEVITDUR, AFTER);
    look(false);
  }
  msg(is_hallucinating(&player)
    ? "oh, wow!  You're floating in the air!"
    : "you start to float in the air");
}

void
remove_levitating()
{
  set_levitating(&player, false);
  msg(is_hallucinating(&player)
    ? "bummer!  You've hit the ground"
    : "you float gently to the ground");
}

void
raise_level()
{
  pstats.s_exp = e_levels[pstats.s_lvl-1] + 1L;
  check_level();
  msg("you suddenly feel much more skillful");
}
