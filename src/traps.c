
#include "status_effects.h"
#include "command.h"
#include "io.h"
#include "armor.h"
#include "fight.h"
#include "colors.h"
#include "list.h"
#include "level.h"
#include "rings.h"
#include "misc.h"

#include "traps.h"


const char *trap_names[NTRAPS] = {
  "a trapdoor",
  "an arrow trap",
  "a sleeping gas trap",
  "a beartrap",
  "a teleport trap",
  "a poison dart trap",
  "a rust trap",
  "a mysterious trap"
};

enum trap_t
be_trapped(THING *target, coord *tc)
{
  PLACE *pp = INDEX(tc->y, tc->x);
  char tr = pp->p_flags & F_TMASK;

  /* If we're levitating, we won't trigger the trap */
  if (is_levitating(target))
    return T_RUST; /* this needs to be neither T_DOOR nor T_TELEP */

  if (target == &player)
  {
    pp->p_ch = TRAP;
    pp->p_flags |= F_SEEN;
    command_stop(true);
  }

  switch (tr)
  {
    case T_DOOR:
      level++;
      level_new();
      msg("you fell into a trap!");
    when T_BEAR:
      become_stuck();
      msg("you are caught in a bear trap");
    when T_MYST:
      switch(rnd(11))
      {
        case 0: msg("you are suddenly in a parallel dimension");
        when 1: msg("the light in here suddenly seems %s", colors_random());
        when 2: msg("you feel a sting in the side of your neck");
        when 3: msg("multi-colored lines swirl around you, then fade");
        when 4: msg("a %s light flashes in your eyes", colors_random());
        when 5: msg("a spike shoots past your ear!");
        when 6: msg("%s sparks dance across your armor", colors_random());
        when 7: msg("you suddenly feel very thirsty");
        when 8: msg("you feel time speed up suddenly");
        when 9: msg("time now seems to be going slower");
        when 10: msg("you pack turns %s!", colors_random());
      }
    when T_SLEEP:
      fall_asleep();
      addmsg("a strange white mist envelops you and ");
    when T_ARROW:
      if (fight_swing_hits(pstats.s_lvl - 1, armor_for_thing(&player), 1))
      {
        pstats.s_hpt -= roll(1, 6);
        if (pstats.s_hpt <= 0)
        {
          msg("an arrow killed you");
          death('a');
        }
        else
          msg("oh no! An arrow shot you");
      }
      else
      {
        THING *arrow = new_item();
        init_weapon(arrow, ARROW);
        arrow->o_count = 1;
        arrow->o_pos = hero;
        fall(arrow, false);
        msg("an arrow shoots past you");
      }
    when T_TELEP: /* Works for monsters */
      teleport(target, NULL);
      if (target == &player)
        mvaddcch(tc->y, tc->x, TRAP); /* Mark trap before we leave */
    when T_DART:
      if (!fight_swing_hits(pstats.s_lvl + 1, armor_for_thing(&player), 1))
        msg("a small dart whizzes by your ear and vanishes");
      else
      {
        pstats.s_hpt -= roll(1, 4);
        if (pstats.s_hpt <= 0)
        {
          msg("a poisoned dart killed you");
          death('d');
        }
        if (!player_has_ring_with_ability(R_SUSTSTR) && !player_save_throw(VS_POISON))
          chg_str(-1);
        msg("a small dart just hit you in the shoulder");
      }
    when T_RUST:
      msg("a gush of water hits you on the head");
      armor_rust();
  }

  flushinp();
  return tr;
}


