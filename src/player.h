#ifndef _ROGUE14_PLAYER_H_
#define _ROGUE14_PLAYER_H_

#include <stdbool.h>

#include "rogue.h"

void *__player_ptr(void);
struct stats max_stats;

/* General */
void player_init(void);
bool player_save_state(void);
bool player_load_state(void);

bool is_player(THING *thing);
int player_save_throw(int which);

/* Status Effects */
bool player_has_true_sight(void);
void player_add_true_sight(bool permanent);
void player_remove_true_sight(void);

bool player_is_confused(void);
void player_set_confused(bool permanent);
void player_remove_confused(void);

bool player_is_held(void);
void player_set_held(void);
void player_remove_held(void);

bool player_can_sense_monsters(void);
void player_add_sense_monsters(bool permanent);
void player_remove_sense_monsters(void);

bool player_is_hallucinating(void);
void player_set_hallucinating(bool permanent);
void player_remove_hallucinating(void);

int player_get_speed(void);
void player_increase_speed(bool permanent);
void player_decrease_speed(void);

bool player_is_running(void);
void player_start_running(void);
void player_stop_running(void);

bool player_is_blind(void);
void player_set_blind(bool permanent);
void player_remove_blind(void);

bool player_is_levitating(void);
void player_start_levitating(bool permanent);
void player_stop_levitating(void);

bool player_has_confusing_attack(void);
void player_set_confusing_attack(void);
void player_remove_confusing_attack(void);

void player_fall_asleep(void);
void player_become_stuck(void);
void player_become_poisoned(void);
void player_teleport(coord *target);
bool player_search(void);

bool player_is_stealthy(void);

/* Position */
int player_y(void);
int player_x(void);
coord *player_get_pos(void);
void player_set_pos(coord *new_pos);

/* Current Room */
struct room *player_get_room(void);
void player_set_room(struct room *new_room);

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

/* Armor */
int player_get_armor(void);

/* Level */
int player_get_level(void);
void player_raise_level(void);
void player_check_for_level_up(void);
void player_lower_level(void);

/* Experience */
int player_get_exp(void);
void player_earn_exp(int amount);

#endif /* _ROGUE14_PLAYER_H_ */
