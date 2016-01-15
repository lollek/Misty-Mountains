#ifndef MONSTER_PRIVATE_H
#define MONSTER_PRIVATE_H

#include <string>
#include <vector>

#include "Coordinate.h"
#include "MonsterList.h"
#include "things.h"

extern MonsterList monster_list;  /* List of monsters on the level */

struct monster_template {
    std::string const  m_name;             /* What to call the monster */
    int                m_carry;            /* Probability of carrying something */
    int                m_flags;            /* things about the monster */
    int                m_basexp;           /* Base xp */
    int                m_level;            /* Level */
    int                m_armor;            /* Armor */
    damage             m_dmg[MAXATTACKS];  /* Monster attacks */
};

extern struct monster_template monsters[26];

/* Find proper destination for monster */
void monster_find_new_target(monster* tp);

void monster_start_chasing(monster* monster);
void monster_set_target(monster* mon, Coordinate* target);

#endif /* MONSTER_PRIVATE_H */
