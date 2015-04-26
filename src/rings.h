#ifndef _ROGUE14_RINGS_H_
#define _ROGUE14_RINGS_H_

enum ring_t {
  R_PROTECT = 0,
  R_ADDSTR = 1,
  R_SUSTSTR = 2,
  R_SEARCH = 3,
  R_SEEINVIS = 4,
  R_NOP = 5,
  R_AGGR = 6,
  R_ADDHIT = 7,
  R_ADDDAM = 8,
  R_REGEN = 9,
  R_DIGEST = 10,
  R_TELEPORT = 11,
  R_STEALTH = 12,
  R_SUSTARM = 13,
  NRINGS
};

/* Variables: TODO: Make these private */
char *r_stones[NRINGS];
struct obj_info ring_info[NRINGS];

bool ring_put_on(void);
bool ring_take_off(void);

int ring_drain_amount(void); /* How much food the player's rings drain */
char *ring_bonus(THING *obj);

#endif /* _ROGUE14_RINGS_H_ */
