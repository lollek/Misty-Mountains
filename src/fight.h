#ifndef _ROGUE14_FIGHT_H_
#define _ROGUE14_FIGHT_H_

/** fight:
 * The player attacks the monster. */
int fight(coord *mp, THING *weap, bool thrown);

/** attack:
 * The monster attacks the player */
int attack(THING *mp);

/** swing:
 * Returns true if the swing hits */
int swing(int at_lvl, int op_arm, int wplus);

/** bounce:
 * A missile misses a monster */
void bounce(THING *weap, const char *mname, bool noend);

#endif /* _ROGUE14_FIGHT_H_ */
