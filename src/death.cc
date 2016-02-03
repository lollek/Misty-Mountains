#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>

#include <string>

using namespace std;

#include "game.h"
#include "io.h"
#include "misc.h"
#include "monster.h"
#include "pack.h"
#include "score.h"

#include "death.h"

char*
death_reason(char buf[], int reason)
{
  if (isupper(reason))
  {
    string monster = monster_name_by_type(static_cast<char>(reason));
    sprintf(buf, "Killed by a%s %s", vowelstr(monster).c_str(), monster.c_str());
  }
  else
  {
    char const* death_reason = "???";
    switch (reason)
    {
      case 'a': case DEATH_ARROW: death_reason = "Pierced by an arrow"; break;
      case 'b': case DEATH_BOLT:  death_reason = "Pierced by a bolt"; break;
      case 'd': case DEATH_DART:  death_reason = "Poisoned by a dart"; break;
      case 'f': case DEATH_FLAME: death_reason = "Burned to crisp"; break;
      case 'h': case 'i': case DEATH_ICE: death_reason = "Frozen solid"; break;
      case 's': case DEATH_HUNGER: death_reason = "Starved to death"; break;

      default:
      case DEATH_UNKNOWN:
        io_debug_fatal("Unknown death reason: %d(%c)", reason, reason);
    }
    sprintf(buf, "%s", death_reason);
  }

  return buf;
}

void
death(int monst)
{
  pack_gold -= pack_gold / 10;

  Game::io->refresh();
  Game::io->message("You die!");
  io_readchar(false);

  pack_evaluate();
  score_show_and_exit(pack_gold, pack_contains_amulet() ? 3 : 0, static_cast<char>(monst));
}


