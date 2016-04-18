#include "game.h"
#include "io.h"
#include "os.h"
#include "player.h"

#include "magic.h"

int magic_hold_nearby_monsters() {
  Coordinate const& player_pos{player->get_position()};
  int monsters_affected{0};
  Monster* held_monster{nullptr};

  for (int x{player_pos.x - 2}; x <= player_pos.x + 2; x++) {
    if (x >= 0 && x < IO::map_width) {
      for (int y{player_pos.y - 2}; y <= player_pos.y + 2; y++) {
        if (y >= 0 && y < IO::map_height) {
          Monster* monster{Game::level->get_monster(x, y)};
          if (monster != nullptr) {
            monster->set_held();
            monsters_affected++;
            held_monster = monster;
          }
        }
      }
    }
  }

  if (monsters_affected == 1) {
    Game::io->message(held_monster->get_name() + " freezes");

  } else if (monsters_affected > 1) {
    Game::io->message("the monsters around you freeze");

  } else { /* monsters_affected == 0 */
    switch (os_rand_range(3)) {
      case 0: Game::io->message("you are unsure if anything happened"); break;
      case 1: Game::io->message("you feel a strange sense of loss"); break;
      case 2: Game::io->message("you feel a powerful aura"); break;
    }
  }
  return monsters_affected;
}
