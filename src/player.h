#pragma once

#include "rogue.h"

class Player : public monster {
public:
  explicit Player();

  Player(Player const&) = delete;
  Player& operator=(Player const&) = delete;

  int speed = 0;

private:
};

extern Player* player;

/* Variable (TODO: remove these) */
extern int          player_turns_without_action; /* Turns asleep */
extern int          player_turns_without_moving; /* Turns held in place */
extern bool         player_alerted;              /* Alert the player? */
extern stats        player_max_stats;            /* Current max stats */

int player_save_throw(int which);

/* Status Effects */
bool player_has_true_sight();
void player_add_true_sight(bool permanent);
void player_remove_true_sight(int unused);

bool player_is_confused();
void player_set_confused(bool permanent);
void player_remove_confused(int unused);

bool player_is_held();
void player_set_held();
void player_remove_held();

bool player_can_sense_monsters();
void player_add_sense_monsters(bool permanent);
void player_remove_sense_monsters(int unused);

bool player_is_hallucinating();
void player_set_hallucinating(bool permanent);
void player_remove_hallucinating(int unused);

int player_get_speed();
void player_increase_speed(bool permanent);
void player_decrease_speed(int unused);

bool player_is_running();
void player_start_running();
void player_stop_running();

bool player_is_blind();
void player_set_blind(bool permanent);
void player_remove_blind(int unused);

bool player_is_levitating();
void player_start_levitating(bool permanent);
void player_stop_levitating(int unused);

bool player_has_confusing_attack();
void player_set_confusing_attack();
void player_remove_confusing_attack();

void player_fall_asleep();
void player_become_stuck();
void player_become_poisoned();
void player_teleport(Coordinate* target);
bool player_search();

bool player_is_stealthy();
bool player_has_ring_with_ability(int ability);

/* Position */
int player_y();
int player_x();
Coordinate* player_get_pos();
void player_set_pos(Coordinate* new_pos);

/* Current Room */
struct room* player_get_room();
void player_set_room(struct room* new_room);

/* Strength */
int player_get_strength();
bool player_strength_is_weakened();
void player_restore_strength();
void player_modify_strength(int amount);

/* Health */
int player_get_health();
int player_get_max_health();
void player_restore_health(int amount, bool can_raise_total);
bool player_is_hurt();
void player_modify_max_health(int amount);
void player_lose_health(int amount);

/* Armor */
int player_get_armor();

/* Level */
int player_get_level();
void player_raise_level();
void player_check_for_level_up();
void player_lower_level();

/* Experience */
int player_get_exp();
void player_earn_exp(int amount);
