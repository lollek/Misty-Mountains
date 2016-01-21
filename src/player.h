#pragma once

#include "rogue.h"
#include "monster.h"

// Structure describing a fighting being
struct stats {
  int                  s_str;   // Strength
  int                  s_exp;   // Experience
  int                  s_lvl;   // level of mastery
  int                  s_arm;   // Armor class
  int                  s_hpt;   // Hit points
  std::vector<damage>  s_dmg;   // Attacks
  int                  s_maxhp; // Max hit points
};

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
  void set_hallucinating() override;
  void set_not_hallucinating() override;
  void set_blind() override;
  void set_not_blind() override;
  void set_levitating() override;
  void set_not_levitating() override;
  void set_confusing_attack() override;

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
  std::string get_attack_string(bool successful_hit) const override;
  std::string get_name() const override;

private:
  bool senses_monsters = false;
  int speed = 0;
};

extern Player* player;

/* Variable (TODO: remove these) */
extern int          player_turns_without_action; /* Turns asleep */
extern int          player_turns_without_moving; /* Turns held in place */
extern bool         player_alerted;              /* Alert the player? */
extern stats        player_max_stats;            /* Current max stats */
