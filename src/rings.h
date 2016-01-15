#pragma once

#include <vector>

/* How many rings we can wield */
#define CONCURRENT_RINGS 2

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

extern std::vector<obj_info> ring_info;

void ring_init();

bool ring_put_on();
bool ring_take_off();

int ring_drain_amount(); /* How much food the player's rings drain */

bool ring_is_known(enum ring_t ring);

void ring_description(item const* item, char* buf);
item* ring_create(int which, bool random_stats);
