#ifndef _ROGUE14_PLAYER_H_
#define _ROGUE14_PLAYER_H_

#include <stdbool.h>

#include "rogue.h"

/* Position */
coord *player_get_pos(void);
void player_set_pos(coord *new_pos);

/* Strength */
int player_get_strength(void);
bool player_strength_is_weakened(void);
void player_restore_strength(void);
void player_modify_strength(int amount);

/* Health */
int player_get_health(void);
int player_get_max_health(void);
void player_restore_health(int amount, bool can_raise_total);
bool player_is_hurt(void);
void player_modify_max_health(int amount);
void player_lose_health(int amount);

/* Level */
int player_get_level(void);
void player_raise_level(void);
void player_check_for_level_up(void);
void player_lower_level(void);

/* Experience */
int player_get_exp(void);
void player_earn_exp(int amount);

/* #define pstats		player.t_stats */

#endif /* _ROGUE14_PLAYER_H_ */
