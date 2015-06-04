#ifndef _MONSTER_PRIVATE_H_
#define _MONSTER_PRIVATE_H_

#include "coord.h"
#include "things.h"

struct monster {
    char const* m_name;   /* What to call the monster */
    int m_carry;          /* Probability of carrying something */
    short m_flags;        /* things about the monster */
    struct stats m_stats; /* Initial stats */
};

extern struct monster monsters[26];

/* Find proper destination for monster */
void monster_find_new_target(THING* tp);

void monster_start_chasing(THING* monster);
void monster_set_target(THING* mon, coord* target);

#endif /* _MONSTER_PRIVATE_H_ */
