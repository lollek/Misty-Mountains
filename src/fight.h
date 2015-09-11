#ifndef ROGUE14_FIGHT_H
#define ROGUE14_FIGHT_H

#include <stdbool.h>

#include "coord.h"
#include "things.h"

/** fight_against_monster:
 * The player attacks the monster. */
int fight_against_monster(coord const* mp, THING* weap, bool thrown);

/** fight_against_player:
 * The monster attacks the player */
int fight_against_player(THING* mp);

/** fight_swing_hits:
 * Returns true if the swing hits */
int fight_swing_hits(int at_lvl, int op_arm, int wplus);

/** fight_missile_miss:
 * A missile misses a monster */
void fight_missile_miss(THING const* weap, char const* mname);

#endif /* ROGUE14_FIGHT_H */
