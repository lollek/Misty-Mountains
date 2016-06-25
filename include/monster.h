#pragma once

#include <istream>
#include <list>
#include <ostream>
#include <string>
#include <vector>

#include "character.h"
#include "coordinate.h"
#include "item.h"

class Monster : public Character {
 public:
  enum Type {
    Bat,
    Goblin,
    Kobold,
    Orc,
    SkeletonHuman,
    Hobgoblin,
    ZombieHuman,

    Quasit,
    Worg,
    Ogre,
    Yeti,
    IceMonster,
    Aquator,
    Troll,

    ShadowDemon,
    Erinyes,
    Nightmare,
    GenieDjinni,
    GenieVizier,
    GenieEfreeti,
    GenieJanni,
    GenieMarid,
    GenieShaitan,

    AdultRedDragon,
    Jabberwock,

    NMONSTERS,
  };

  Monster(Type subtype, Coordinate const& pos);
  Monster(std::istream&);

  ~Monster();

  Monster& operator=(Monster&&) = default;

  void save(std::ostream&) const override;
  bool load(std::istream&) override;

  // Setters
  void set_invisible() override;
  void set_target(Coordinate const* target);
  void set_disguise(char);

  // Modifiers
  void find_new_target();
  void notice_player();

  // monster_chase.c
  bool take_turn();  // True if monster is still alive

  // Getters
  std::string get_attack_string(bool successful_hit) const override;
  std::string get_name() const override;
  char get_look() const;
  char get_disguise() const;
  Type get_subtype() const;
  Coordinate const* get_target() const;
  std::list<Item*>& get_pack();

  // Statics
  static std::string name(Type type);
  static void all_move();
  static Type random_monster_type_for_level();
  static Type random_monster_type();

 private:
  char look;
  char disguise;
  Type subtype;
  Coordinate const* target;
  std::list<Item*> pack;

  // monster_constructors.cc
  Monster& operator=(Monster const&) = delete;  // They will share inventory
  Monster(Monster const&) = delete;             // They will share inventory
  Monster(int str, int dex, int con, int int_, int wis, int cha, int exp,
          int lvl, int ac, int hp, std::vector<damage> const& dmg,
          Coordinate const& pos, std::vector<Feat> const& feats, int speed,
          Race race, char look, char disguise, Type subtype,
          Coordinate const* target, std::list<Item*> pack);

  static Monster bat();
  static Monster kobold();
  static Monster orc();
  static Monster goblin();
  static Monster skeleton_human();
  static Monster zombie_human();
  static Monster hobgoblin();

  static Monster quasit();
  static Monster worg();
  static Monster ogre();
  static Monster yeti();
  static Monster ice_monster();
  static Monster aquator();
  static Monster troll();

  static Monster erinyes();
  static Monster shadow_demon();
  static Monster nightmare();
  static Monster genie_djinni();
  static Monster genie_vizier();
  static Monster genie_efreeti();
  static Monster genie_janni();
  static Monster genie_marid();
  static Monster genie_shaitan();

  static Monster adult_red_dragon();
  static Monster jabberwock();

  static unsigned long long constexpr TAG_MONSTER = 0xd000000000000001ULL;
  static unsigned long long constexpr TAG_MISC = 0xd000000000000002ULL;
};

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
