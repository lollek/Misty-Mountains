#pragma once

#include <vector>

#include "damage.h"
#include "coordinate.h"

class Character {
public:
  enum Race : int {
    Human,
    Dwarf,
    Elf,

    Animal,
    Reptilian,
    Goblinoid,
    Elemental,
    Fey,
    Orc,
    Undead,
    Humanoid,
    MonstrousHumanoid,
    Aberration,
    MagicBeast,
    Dragon,
    Outsider,
    Giant,
  };

  enum Feat : int {
    // Permanent features
    AttacksOnSight,
    RandomMoveD2,           // "Confused" when moving if 1d2 == 1
    MoveThroughStone,       // Moves through stone like air
    PermanentlyInvisible,
    AttackRuinsMetal,
    AttackFreezesTarget,
    Regenerating5,          // 5HP per round

    // Activate-able for free
    BreatheConeFireSmall,   // 40ft, DC 19, 6d10 fire
    BreatheConeFireMedium,  // 50ft, DC 24, 12d10 fire
    BreatheConeFireLarge,   // 60ft, DC 30, 20d10 fire
    Planeshift,             // Can planeshift to escape
    TurnInvisible,          // Can turn invisible at will
    ScorchingRay,           // CL11, 3 rays, 4d6 fire each
  };

  virtual ~Character() = default;

  virtual std::string get_attack_string(bool successful_hit) const = 0;
  virtual std::string get_name() const = 0;

  // Getters
  virtual int                        get_strength() const;
  virtual int                        get_default_strength() const;
  virtual int                        get_dexterity() const;
  virtual int                        get_default_dexterity() const;
  virtual int                        get_constitution() const;
  virtual int                        get_default_constitution() const;
  virtual int                        get_intelligence() const;
  virtual int                        get_default_intelligence() const;
  virtual int                        get_wisdom() const;
  virtual int                        get_default_wisdom() const;
  virtual int                        get_charisma() const;
  virtual int                        get_default_charisma() const;
  virtual int                        get_experience() const;
  virtual int                        get_level() const;
  virtual int                        get_ac() const;
  virtual int                        get_health() const;
  virtual int                        get_max_health() const;
  virtual Coordinate const&          get_position() const;
  virtual std::vector<damage> const& get_attacks() const;
  virtual Race                       get_race() const;

  // Setters
  virtual void set_position(Coordinate const& position);
  virtual void set_base_stats(int strength, int dexterity, int constitution,
                              int intelligence, int wisdom, int charisma);

  // Modifiers
  virtual void increase_speed();
  virtual void decrease_speed();
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
  virtual int  get_moves_this_round();
  virtual int  get_speed() const;
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
  virtual bool is_greedy() const;
  virtual bool is_players_target() const;
  virtual bool attack_freezes() const;
  virtual bool attack_damages_armor() const;
  virtual bool attack_steals_gold() const;
  virtual bool attack_steals_item() const;
  virtual bool attack_drains_strength() const;
  virtual bool attack_drains_health() const;
  virtual bool attack_drains_experience() const;
  virtual bool attacks_on_sight() const;

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
  virtual void set_running();
  virtual void set_not_running();

  virtual void  save(std::ostream&) const;
  virtual bool  load(std::istream&);

  static std::string race_to_string(Race race);


protected:
  explicit Character(int strength, int dexterity, int constitution, int intelligence,
      int wisdom, int charisma, int experience, int level, int armor,
      int health, std::vector<damage> const& attacks, Coordinate const& position,
      std::vector<Feat> const& feats, int speed, Race race);

  explicit Character() = default;
  explicit Character(Character const&) = default;
  explicit Character(Character&&) = default;
  Character& operator=(Character const&) = default;

  virtual bool has_feat(Feat feat) const;


private:
  // Stats
  int                  strength;
  int                  default_strength;
  int                  dexterity;
  int                  default_dexterity;
  int                  constitution;
  int                  default_constitution;
  int                  intelligence;
  int                  default_intelligence;
  int                  wisdom;
  int                  default_wisdom;
  int                  charisma;
  int                  default_charisma;

  int                  experience;
  int                  level;
  int                  base_ac;
  int                  hit_dice;
  int                  base_health;
  int                  health;
  std::vector<damage>  attacks;
  Coordinate           position;
  int                  speed;
  Race                 race;
  std::vector<Feat>    feats;
  int                  turns_not_moved;

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
  bool stuck;
  bool attack_freeze;
  bool attack_damage_armor;
  bool attack_steal_gold;
  bool attack_steal_item;
  bool attack_drain_strength;
  bool attack_drain_health;
  bool attack_drain_experience;


  static unsigned long long constexpr TAG_CHARACTER       = 0x8000000000000001ULL;
  static unsigned long long constexpr TAG_STATS           = 0x8000000000000002ULL;
  static unsigned long long constexpr TAG_MISC            = 0x8000000000000003ULL;
  static unsigned long long constexpr TAG_FLAG            = 0x8000000000000004ULL;
};

