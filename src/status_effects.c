
#include "status_effects.h"
#include "rogue.h"

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
    come_down();
  }
}

void
become_confused(bool permanent)
{
  if (on(player, ISHUH))
    lengthen(unconfuse, HUHDURATION);
  else
  {
    player.t_flags |= ISHUH;
    if (!permanent)
      fuse(unconfuse, 0, HUHDURATION, AFTER);
    look(FALSE);
  }
  msg(is_hallucinating(player)
    ? "what a trippy feeling!"
    : "wait, what's going on here. Huh? What? Who?");
}

void
become_healed()
{
  if ((pstats.s_hpt += roll(pstats.s_lvl, 4)) > max_hp)
    pstats.s_hpt = ++max_hp;
  sight();
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
  sight();
  come_down();
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
    fuse((void(*)())turn_see, TRUE, MFINDDURATION, AFTER);
  /* FIXME: Make sure that this work */
  if (!turn_see(FALSE))
    msg("you have a %s feeling for a moment, then it passes",
        is_hallucinating(player) ? "normal" : "strange");
}

void
become_tripping(bool permanent)
{
  if (is_hallucinating(player))
    lengthen(come_down, SEEDURATION);
  else
  {
    if (on(player, SEEMONST))
      turn_see(FALSE);
    start_daemon(visuals, 0, BEFORE);
    seenstairs = seen_stairs();
    player.t_flags |= ISHALU;
    if (!permanent)
      fuse(come_down, 0, SEEDURATION, AFTER);
    look(FALSE);
  }
  msg("Oh, wow!  Everything seems so cosmic!");
}

void
become_true_seeing(bool permanent)
{
  if (on(player, CANSEE))
    lengthen(unsee, SEEDURATION);
  else
  {
    player.t_flags |= CANSEE;
    if (!permanent)
      fuse(unsee, 0, SEEDURATION, AFTER);
    look(FALSE);
  }
  msg("everything suddenly looks sharper");
  sight();
}

void
become_hasted(bool permanent)
{
  after = FALSE;
  if (on(player, ISHASTE))
  {
    no_command += rnd(8);
    player.t_flags &= ~(ISRUN|ISHASTE);
    extinguish(nohaste);
    msg("you faint from exhaustion");
    return;
  }
  else
  {
    player.t_flags |= ISHASTE;
    if (!permanent)
      fuse(nohaste, 0, HASTEDURATION, AFTER);
    msg("you feel yourself moving much faster");
  }
}

void become_blind(bool permanent)
{
  if (is_blind(player))
    lengthen(sight, SEEDURATION);
  else
  {
    player.t_flags |= ISBLIND;
    if (!permanent)
      fuse(sight, 0, SEEDURATION, AFTER);
    look(FALSE);
  }
  msg(is_hallucinating(player)
    ? "oh, bummer!  Everything is dark!  Help!"
    : "a cloak of darkness falls around you");
}

void become_levitating(bool permanent)
{
  if (is_levitating(player))
    lengthen(land, HEALTIME);
  else
  {
    player.t_flags |= ISLEVIT;
    if (!permanent)
      fuse(land, 0, HEALTIME, AFTER);
    look(FALSE);
  }
  msg(is_hallucinating(player)
    ? "oh, wow!  You're floating in the air!"
    : "you start to float in the air");
}

void
raise_level()
{
  pstats.s_exp = e_levels[pstats.s_lvl-1] + 1L;
  check_level();
  msg("you suddenly feel much more skillful");
}
