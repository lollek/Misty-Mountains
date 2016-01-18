#pragma once

#include <string>
#include <list>
#include <vector>

#include "Coordinate.h"
#include "things.h"

struct monster_template {
    std::string const    m_name;    /* What to call the monster */
    int                  m_carry;   /* Probability of carrying something */
    int                  m_flags;   /* things about the monster */
    int                  m_basexp;  /* Base xp */
    int                  m_level;   /* Level */
    int                  m_armor;   /* Armor */
    std::vector<damage>  m_dmg;     /* Monster attacks */
};

extern struct monster_template monsters[26];

/* Find proper destination for monster */
void monster_find_new_target(Monster* tp);

void monster_start_chasing(Monster* monster);
void monster_set_target(Monster* mon, Coordinate const& target);
