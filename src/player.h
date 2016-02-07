#pragma once

#include "rogue.h"
#include "monster.h"

class Player : public Character {
public:
  explicit Player();

  Player(Player const&) = delete;
  Player& operator=(Player const&) = delete;

  // Getters
  int get_armor() const override;
  bool has_true_sight() const override;
  bool can_sense_monsters() const;
  int get_speed() const;
  bool is_stealthy() const;
  int get_strength_with_bonuses() const;
  struct room* get_previous_room() const;

  // Modifier
  void increase_speed();
  void decrease_speed();
  void restore_strength() override;
  void modify_strength(int amount) override;
  void raise_level(int amount) override;

  // Setters
  void set_sense_monsters();
  void remove_sense_monsters();
  void set_true_sight() override;
  void remove_true_sight() override;
  void set_confused() override;
  void set_not_confused() override;
  void set_blind() override;
  void set_not_blind() override;
  void set_levitating() override;
  void set_not_levitating() override;
  void set_confusing_attack() override;
  void set_previous_room(struct room* room);

  // player_food.cc
  void         eat();
  void         digest_food();
  std::string  get_hunger_state() const;
  int          get_nutrition_left() const;
  static int   get_starting_nutrition();

  // Misc
  bool saving_throw(int which) const;
  bool has_ring_with_ability(int ability) const;
  void check_for_level_up();
  void waste_time(int rounds) const;
  void fall_asleep();
  void become_stuck();
  void become_poisoned();
  void teleport(Coordinate const* target); // random spot if target is nullptr
  bool has_seen_stairs() const;
  bool can_see(Coordinate const& coord) const;
  void search();
  void rust_armor();
  std::string get_attack_string(bool successful_hit) const override;
  std::string get_name() const override;

private:
  struct room* previous_room;
  bool         senses_monsters;
  int          speed;

  int          nutrition_left;
};

extern Player* player;

/* Variable (TODO: remove these) */
extern int          player_turns_without_action; /* Turns asleep */
extern int          player_turns_without_moving; /* Turns held in place */
extern bool         player_alerted;              /* Alert the player? */
