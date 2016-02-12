#pragma once

#include <vector>

#include "damage.h"
#include "coordinate.h"

class Character {
public:
  virtual ~Character() = default;

  virtual std::string get_attack_string(bool successful_hit) const = 0;
  virtual std::string get_name() const = 0;

  // Getters
  int                        get_strength() const;
  int                        get_default_strength() const;
  int                        get_experience() const;
  int                        get_level() const;
  virtual int                get_armor() const;
  int                        get_health() const;
  int                        get_max_health() const;
  Coordinate const&          get_position() const;
  std::vector<damage> const& get_attacks() const;
  int                        get_type() const;

  // Setters
  virtual void set_position(Coordinate const& position);

  // Modifiers
  virtual void increase_speed() = 0;
  virtual void decrease_speed() = 0;
  virtual void take_damage(int damage);
  virtual void gain_experience(int experience);
  virtual void restore_strength();
  virtual void modify_strength(int amount);
  virtual void modify_max_health(int amount);
  virtual void restore_health(int amount, bool can_raise_max_health);
  virtual void raise_level(int amount);
  virtual void lower_level(int amount);

  // Returns a coord to somewhere we could possibly walk
  virtual Coordinate possible_random_move();
  virtual bool is_hurt() const;

  // Flag getters
  virtual bool is_blind() const;
  virtual bool is_cancelled() const;
  virtual bool is_confused() const;
  virtual bool has_confusing_attack() const;
  virtual bool is_found() const;
  virtual bool is_invisible() const;
  virtual bool is_levitating() const;
  virtual bool has_true_sight() const;
  virtual bool is_held() const;
  virtual bool is_stuck() const;
  virtual bool is_chasing() const;
  virtual bool is_running() const;
  virtual bool is_mean() const;
  virtual bool is_greedy() const;
  virtual bool is_players_target() const;
  virtual bool is_flying() const;
  virtual bool attack_freezes() const;
  virtual bool attack_damages_armor() const;
  virtual bool attack_steals_gold() const;
  virtual bool attack_steals_item() const;
  virtual bool attack_drains_strength() const;
  virtual bool attack_drains_health() const;
  virtual bool attack_drains_experience() const;

  // Flag setters
  virtual void set_blind();
  virtual void set_not_blind();
  virtual void set_cancelled();
  virtual void set_not_cancelled();
  virtual void set_confused();
  virtual void set_not_confused();
  virtual void set_confusing_attack();
  virtual void remove_confusing_attack();
  virtual void set_found();
  virtual void set_not_found();
  virtual void set_invisible();
  virtual void set_not_invisible();
  virtual void set_levitating();
  virtual void set_not_levitating();
  virtual void set_true_sight();
  virtual void remove_true_sight();
  virtual void set_held();
  virtual void set_not_held();
  virtual void set_stuck();
  virtual void set_not_stuck();
  virtual void set_chasing();
  virtual void set_not_chasing();
  virtual void set_mean();
  virtual void set_not_mean();
  virtual void set_greedy();
  virtual void set_not_greedy();
  virtual void set_players_target();
  virtual void set_not_players_target();
  virtual void set_flying();
  virtual void set_not_flying();
  virtual void set_running();
  virtual void set_not_running();

  virtual void  save(std::ofstream&) const;
  virtual bool  load(std::ifstream&);


protected:
  Character(int strength, int experience, int level, int armor, int health,
            std::vector<damage> const& attacks, Coordinate const& position,
            unsigned long long flags, char type);

  explicit Character(Character const&) = default;
  explicit Character(Character&&) = default;
  Character& operator=(Character const&) = default;


private:
  int                  strength;
  int                  default_strength;
  int                  experience;
  int                  level;
  int                  armor;
  int                  health;
  std::vector<damage>  attacks;
  int                  max_health;
  Coordinate           position;
  char                 type;

  // Flags
  bool confusing_attack;
  bool true_sight;
  bool blind;
  bool cancelled;
  bool levitating;
  bool found;
  bool greedy;
  bool players_target;
  bool held;
  bool confused;
  bool invisible;
  bool mean;
  bool regenerating;
  bool running;
  bool flying;
  bool stuck;
  bool attack_freeze;
  bool attack_damage_armor;
  bool attack_steal_gold;
  bool attack_steal_item;
  bool attack_drain_strength;
  bool attack_drain_health;
  bool attack_drain_experience;


  static unsigned long long constexpr TAG_CHARACTER       = 0x8000000000000000ULL;
};

