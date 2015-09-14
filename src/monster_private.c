#include "coord.h"
#include "things.h"
#include "player.h"
#include "level.h"
#include "scrolls.h"
#include "misc.h"
#include "io.h"
#include "os.h"

#include "monster.h"
#include "monster_private.h"

void
monster_find_new_target(THING* monster)
{
  int prob = monsters[monster->t.t_type - 'A'].m_carry;
  if (prob <= 0 || monster->t.t_room == player_get_room()
      || monster_seen_by_player(&monster->t))
  {
    monster_set_target(monster, player_get_pos());
    return;
  }

  for (THING* obj = level_items; obj != NULL; obj = obj->o.l_next)
  {
    if (obj->o.o_type == SCROLL && obj->o.o_which == S_SCARE)
      continue;

    if (roomin(&obj->o.o_pos) == monster->t.t_room && os_rand_range(100) < prob)
    {
      THING *someone;
      for (someone = monster_list; someone != NULL; someone = someone->t.l_next)
        if (someone->t.t_dest == &obj->o.o_pos)
          break;

      if (someone == NULL)
      {
        monster_set_target(monster, &obj->o.o_pos);
        return;
      }
    }
  }

  monster_set_target(monster, player_get_pos());
}

void
monster_start_chasing(THING* mon)
{
  mon->t.t_flags |= ISRUN;
}

void
monster_set_target(THING* mon, coord* target)
{
  mon->t.t_dest = target;
}
