#ifndef MONSTER_PRIVATE_H
#define MONSTER_PRIVATE_H

#include "coord.h"
#include "things.h"

extern THING* monster_list;  /* List of monsters on the level */

struct monster_template {
    char const*   m_name;             /* What to call the monster */
    int           m_carry;            /* Probability of carrying something */
    int           m_flags;            /* things about the monster */
    int           m_basexp;           /* Base xp */
    int           m_level;            /* Level */
    int           m_armor;            /* Armor */
    struct damage m_dmg[MAXATTACKS];  /* Monster attacks */
};

extern struct monster_template monsters[26];

/* Find proper destination for monster */
void monster_find_new_target(THING* tp);

void monster_start_chasing(THING* monster);
void monster_set_target(THING* mon, coord* target);

#endif /* MONSTER_PRIVATE_H */
