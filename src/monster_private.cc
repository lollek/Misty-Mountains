#include <list>

using namespace std;

#include "Coordinate.h"
#include "things.h"
#include "player.h"
#include "level.h"
#include "scrolls.h"
#include "misc.h"
#include "io.h"
#include "os.h"

#include "monster.h"
#include "monster_private.h"

list<Monster*> monster_list;

void
monster_find_new_target(Monster* monster)
{
  int prob = monsters[monster->t_type - 'A'].m_carry;
  if (prob <= 0 || monster->t_room == player_get_room()
      || monster_seen_by_player(monster)) {
    monster_set_target(monster, *player_get_pos());
    return;
  }

  for (Item* obj : level_items) {
    if (obj->o_type == SCROLL && obj->o_which == S_SCARE)
      continue;

    if (roomin(&obj->get_pos()) == monster->t_room && os_rand_range(100) < prob)
    {
      auto result = find_if(monster_list.cbegin(), monster_list.cend(),
          [&] (Monster const* m) {
          return m->t_dest == obj->get_pos();
      });

      if (result == monster_list.cend()) {
        monster_set_target(monster, obj->get_pos());
        return;
      }
    }
  }

  monster_set_target(monster, *player_get_pos());
}

void
monster_start_chasing(Monster* mon)
{
  mon->t_flags |= ISRUN;
}

void
monster_set_target(Monster* mon, Coordinate const& target)
{
  mon->t_dest = target;
}
