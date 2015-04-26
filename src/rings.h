#ifndef _ROGUE14_RINGS_H_
#define _ROGUE14_RINGS_H_

bool ring_put_on(void);
bool ring_take_off(void);

int ring_drain_amount(void); /* How much food the player's rings drain */
char *ring_bonus(THING *obj);

#endif /* _ROGUE14_RINGS_H_ */
