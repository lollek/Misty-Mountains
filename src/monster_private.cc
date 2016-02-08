#include <list>

using namespace std;

#include "Coordinate.h"
#include "player.h"
#include "level.h"
#include "scrolls.h"
#include "misc.h"
#include "io.h"
#include "os.h"
#include "game.h"

#include "monster.h"

void
monster_find_new_target(Monster* monster)
{
  int prob = Monster::monsters->at(static_cast<size_t>(monster->get_type() - 'A')).m_carry;
  if (prob <= 0 || monster->get_room() == player->get_room()
      || monster_seen_by_player(monster)) {
    monster->set_target(&player->get_position());
    return;
  }

  for (Item* obj : Game::level->items) {
    if (obj->o_type == IO::Scroll && obj->o_which == Scroll::SCARE)
      continue;

    if (Game::level->get_room(obj->get_position()) == monster->get_room() &&
        os_rand_range(100) < prob)
    {
      auto result = find_if(Game::level->monsters.cbegin(), Game::level->monsters.cend(),
          [&] (Monster const* m) {
          return m->t_dest == &obj->get_position();
      });

      if (result == Game::level->monsters.cend()) {
        monster->set_target(&obj->get_position());
        return;
      }
    }
  }

  monster->set_target(&player->get_position());
}
