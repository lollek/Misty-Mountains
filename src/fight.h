#pragma once

#include <string>

#include "Coordinate.h"
#include "things.h"

/** fight_against_monster:
 * The player attacks the monster. */
int fight_against_monster(Coordinate const* mp, Item* weap, bool thrown);

/** fight_against_player:
 * The monster attacks the player */
int fight_against_player(monster* mp);

/** fight_swing_hits:
 * Returns true if the swing hits */
int fight_swing_hits(int at_lvl, int op_arm, int wplus);

/** fight_missile_miss:
 * A missile misses a monster */
void fight_missile_miss(Item const* weap, std::string const& mname);
