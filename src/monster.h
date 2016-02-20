#pragma once

#include <string>
#include <list>
#include <vector>

#include "character.h"
#include "coordinate.h"
#include "item.h"
#include "rogue.h"

class Monster : public Character {
public:
  enum Type {
    Aquator, Bat, Centaur, Dragon, Goblin, Flytrap, Griffin, Hobgoblin,
    IceMonster, Jabberwock, Kobold, Leprechaun, Nymph, Orc,
    Phantom, Quagga, Rattlesnake, Snake, Troll, BlackUnicorn, Vampire,
    Wraith, Xeroc, Yeti, Zombie,

    NMONSTERS,
  };
  struct Template {
    std::string const    m_name;    // What to call the monster
    Type                 m_subtype; // Monster subtype
    char                 m_char;    // Monster character on screen
    int                  m_startlvl; // Start spawning at level (inclusive)
    int                  m_stoplvl; // Stop spawning at level (inclusive)
    int                  m_carry;   // Probability of carrying something
    unsigned long long   m_flags;   // things about the monster
    int                  m_speed;   // Moves per turn
    int                  m_basexp;  // Base xp
    int                  m_level;   // Level
    int                  m_armor;   // Armor
    std::vector<damage>  m_dmg;     // Monster attacks
  };



  Monster(Type subtype, Coordinate const& pos);
  Monster(Monster const&) = delete; // Deleted since they would share inventory

  ~Monster();

  Monster& operator=(Monster const&) = delete; // Deleted since they would share inventory
  Monster& operator=(Monster&&) = default;

  // Setters
  void set_invisible() override;
  void set_target(Coordinate const* target);
  void set_disguise(char);

  // Modifiers
  void give_pack();
  void find_new_target();
  void notice_player();
  void increase_speed() override;
  void decrease_speed() override;

  // monster_chase.c
  bool take_turn(); // True if monster is still alive

  // Getters
  int               get_armor() const override;
  std::string       get_attack_string(bool successful_hit) const override;
  std::string       get_name() const override;
  char              get_disguise() const;
  int               get_speed() const;
  Coordinate const* get_target() const;
  Type              get_subtype() const;

  // Statics
  static void                 init_monsters();
  static void                 free_monsters();
  static std::string const&   name(Type type);
  static void                 all_move();
  static Template const&      monster_data(Type type);
  static Type                 random_monster_type_for_level();
  static Type                 random_monster_type();

  // Variables (TODO: Make these private)
  std::list<Item*>   t_pack;    // What the thing is carrying

  int                turns_not_moved;

private:
  char               disguise;
  Type               subtype;
  int                speed;
  Coordinate const*  target;

  static std::vector<Template> const* monsters;

  Monster(Coordinate const& pos, Template const& m_template);
};


/* Variables, TODO: Remove these */
extern int    monster_flytrap_hit;


/* See if a creature save against something */
int monster_save_throw(int which, Monster const* mon);

/* Make monster start running (towards hero?) */
void monster_start_running(Coordinate const* runner);

/* Called to put a monster to death */
void monster_on_death(Monster** monster, bool print_death_message);

// Remove a monster from the screen
// NOTE: monster will be nullptr after this
void monster_remove_from_screen(Monster** monster, bool was_killed);

void monster_teleport(Monster* monster, Coordinate const* destination);

void monster_do_special_ability(Monster** monster);

/* Is any monster seen by the player? */
bool monster_is_anyone_seen_by_player(void);
/* Make all monsters start chasing the player */
void monster_aggravate_all(void);
/* Does any monster desire this item? If so, aggro player */
void monster_aggro_all_which_desire_item(Item* item);

/* Transform the monster into something else */
void monster_polymorph(Monster* monster);

// Attempt to breathe fire on player. True if it tried, false when it didnt
bool monster_try_breathe_fire_on_player(Monster const& monster);

