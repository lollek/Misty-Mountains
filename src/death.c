#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>

#include "io.h"
#include "pack.h"
#include "monster.h"
#include "misc.h"
#include "rip.h"

#include "death.h"

char*
death_reason(char buf[], int reason)
{
  if (isupper(reason))
  {
    char const* monster = monster_name_by_type((char)reason);
    sprintf(buf, "Killed by a%s %s", vowelstr(monster), monster);
  }
  else
  {
    char const* death_reason = NULL;
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
        (void)fail("Unknown death reason: %d(%c)", reason, reason);
        assert(0);
        death_reason = "???";
    }
    sprintf(buf, "%s", death_reason);
  }

  return buf;
}

void
death(int monst)
{
  pack_gold -= pack_gold / 10;

  status();
  refresh();
  msg("You die!");
  readchar(false);

  pack_evaluate();
  score(pack_gold, pack_contains_amulet() ? 3 : 0, (char)monst);
  exit(0);
}


