#ifndef _ROGUE14_FIGHT_H_
#define _ROGUE14_FIGHT_H_

/** fight_against_monster:
 * The player attacks the monster. */
int fight_against_monster(coord *mp, THING *weap, bool thrown);

/** fight_against_player:
 * The monster attacks the player */
int fight_against_player(THING *mp);

/** fight_swing_hits:
 * Returns true if the swing hits */
int fight_swing_hits(int at_lvl, int op_arm, int wplus);

/** fight_missile_miss:
 * A missile misses a monster */
void fight_missile_miss(THING *weap, const char *mname, bool noend);

#endif /* _ROGUE14_FIGHT_H_ */