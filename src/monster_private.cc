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
#include "game.h"

#include "monster.h"
#include "monster_private.h"

void
monster_find_new_target(Monster* monster)
{
  int prob = monsters.at(static_cast<size_t>(monster->get_type() - 'A')).m_carry;
  if (prob <= 0 || monster->get_room() == player->get_room()
      || monster_seen_by_player(monster)) {
    monster_set_target(monster, player->get_position());
    return;
  }

  for (Item* obj : Game::level->items) {
    if (obj->o_type == SCROLL && obj->o_which == S_SCARE)
      continue;

    if (Game::level->get_room(obj->get_pos()) == monster->get_room() &&
        os_rand_range(100) < prob)
    {
      auto result = find_if(Game::level->monsters.cbegin(), Game::level->monsters.cend(),
          [&] (Monster const* m) {
          return m->t_dest == obj->get_pos();
      });

      if (result == Game::level->monsters.cend()) {
        monster_set_target(monster, obj->get_pos());
        return;
      }
    }
  }

  monster_set_target(monster, player->get_position());
}

void
monster_set_target(Monster* mon, Coordinate const& target)
{
  mon->t_dest = target;
}
