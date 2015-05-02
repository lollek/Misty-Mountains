
#include "command.h"
#include "io.h"
#include "armor.h"
#include "fight.h"
#include "colors.h"
#include "list.h"
#include "level.h"
#include "rings.h"
#include "misc.h"
#include "weapons.h"
#include "player.h"

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
be_trapped(coord *tc)
{
  PLACE *pp = INDEX(tc->y, tc->x);
  char tr = pp->p_flags & F_TMASK;

  /* If we're levitating, we won't trigger the trap */
  if (player_is_levitating())
    return T_RUST; /* this needs to be neither T_DOOR nor T_TELEP */

  pp->p_ch = TRAP;
  pp->p_flags |= F_SEEN;
  command_stop(true);

  switch (tr)
  {
    case T_DOOR:
      level++;
      level_new();
      msg("you fell into a trap!");
      break;
    case T_BEAR:
      player_become_stuck();
      msg("you are caught in a bear trap");
      break;
    case T_MYST:
      switch(rnd(11))
      {
        case 0: msg("you are suddenly in a parallel dimension"); break;
        case 1: msg("the light in here suddenly seems %s", colors_random()); break;
        case 2: msg("you feel a sting in the side of your neck"); break;
        case 3: msg("multi-colored lines swirl around you, then fade"); break;
        case 4: msg("a %s light flashes in your eyes", colors_random()); break;
        case 5: msg("a spike shoots past your ear!"); break;
        case 6: msg("%s sparks dance across your armor", colors_random()); break;
        case 7: msg("you suddenly feel very thirsty"); break;
        case 8: msg("you feel time speed up suddenly"); break;
        case 9: msg("time now seems to be going slower"); break;
        case 10: msg("you pack turns %s!", colors_random()); break;
      }
      break;
    case T_SLEEP:
      player_fall_asleep();
      addmsg("a strange white mist envelops you and ");
      break;
    case T_ARROW:
      if (fight_swing_hits(player_get_level() - 1, player_get_armor(), 1))
      {
        player_lose_health(roll(1, 6));
        if (player_get_health() <= 0)
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
        arrow->o_pos = *player_get_pos();
        fall(arrow, false);
        msg("an arrow shoots past you");
      }
      break;
    case T_TELEP: /* Works for monsters */
      player_teleport(NULL);
      mvaddcch(tc->y, tc->x, TRAP); /* Mark trap before we leave */
      break;
    case T_DART:
      if (!fight_swing_hits(player_get_level() + 1, player_get_armor(), 1))
        msg("a small dart whizzes by your ear and vanishes");
      else
      {
        player_lose_health(roll(1, 4));
        if (player_get_health() <= 0)
        {
          msg("a poisoned dart killed you");
          death('d');
        }
        if (!player_has_ring_with_ability(R_SUSTSTR) && !player_save_throw(VS_POISON))
          player_modify_strength(-1);
        msg("a small dart just hit you in the shoulder");
      }
      break;
    case T_RUST:
      msg("a gush of water hits you on the head");
      armor_rust();
      break;
  }

  flushinp();
  return tr;
}


