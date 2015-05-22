#include "coord.h"
#include "things.h"

#include "player.h"
#include "level.h"
#include "scrolls.h"
#include "misc.h"

#include "monster.h"
#include "monster_private.h"

void
monster_find_new_target(THING* monster)
{
  int prob = monsters[monster->t_type - 'A'].m_carry;
  if (prob <= 0 || monster->t_room == player_get_room() || see_monst(monster))
  {
    monster_set_target(monster, player_get_pos());
    return;
  }

  for (THING* obj = lvl_obj; obj != NULL; obj = obj->l_next)
  {
    if (obj->o_type == SCROLL && obj->o_which == S_SCARE)
      continue;

    if (roomin(&obj->o_pos) == monster->t_room && rnd(100) < prob)
    {
      THING *someone;
      for (someone = mlist; someone != NULL; someone = someone->l_next)
        if (someone->t_dest == &obj->o_pos)
          break;

      if (someone == NULL)
      {
        monster_set_target(monster, &obj->o_pos);
        return;
      }
    }
  }

  monster_set_target(monster, player_get_pos());
}

