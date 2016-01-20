#pragma once

#include <vector>

#include "damage.h"
#include "coordinate.h"
#include "level_rooms.h"

// flags for characters
#define CANHUH	0000001		/* creature can confuse */
#define CANSEE	0000002		/* creature can see invisible creatures */
#define ISBLIND	0000004		/* creature is blind */
#define ISCANC	0000010		/* creature has special qualities cancelled */
#define ISLEVIT	0000010		/* hero is levitating */
#define ISFOUND	0000020		/* creature has been seen (used for objects) */
#define ISGREED	0000040		/* creature runs to protect gold */
#define ISHASTE	0000100		/* creature has been hastened */
#define ISTARGET 000200		/* creature is the target of an 'f' command */
#define ISHELD	0000400		/* creature has been held */
#define ISHUH	0001000		/* creature is confused */
#define ISINVIS	0002000		/* creature is invisible */
#define ISMEAN	0004000		/* creature can wake when player enters room */
#define ISHALU	0004000		/* hero is on acid trip */
#define ISREGEN	0010000		/* creature can regenerate */
#define ISRUN	0020000		/* creature is running at the player */
#define ISFLY	0040000		/* creature can fly */
#define ISSLOW	0100000		/* creature has been slowed */
#define ISSTUCK	0200000		/* creature cannot move its feet */




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
  room*                      get_room() const;
  std::vector<damage> const& get_attacks() const;
  int                        get_type() const;

  // Setters
  virtual void set_room(room* new_room);
  virtual void set_position(Coordinate const& position);

  // Modifiers
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
  virtual bool is_hallucinating() const;
  virtual bool is_invisible() const;
  virtual bool is_levitating() const;
  virtual bool has_true_sight() const;
  virtual bool is_held() const;
  virtual bool is_stuck() const;
  virtual bool is_chasing() const;
  virtual bool is_mean() const;
  virtual bool is_greedy() const;
  virtual bool is_players_target() const;
  virtual bool is_slowed() const;
  virtual bool is_hasted() const;
  virtual bool is_flying() const;

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
  virtual void set_hallucinating();
  virtual void set_not_hallucinating();
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
  virtual void set_slowed();
  virtual void set_not_slowed();
  virtual void set_hasted();
  virtual void set_not_hasted();
  virtual void set_flying();
  virtual void set_not_flying();
  virtual void set_running();
  virtual void set_not_running();



protected:
  Character(int strength, int experience, int level, int armor, int health,
            std::vector<damage> const& attacks, Coordinate const& position,
            room* room, int flags, char type);

  Character(Character const&) = default;
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
  room*                room;
  int                  flags;
  char                 type;
};

