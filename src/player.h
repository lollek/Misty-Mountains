#pragma once

#include <istream>
#include <ostream>

#include "rings.h"
#include "weapons.h"
#include "food.h"
#include "monster.h"

enum Equipment {
  Armor,
  Weapon,
  BackupWeapon,
  Ring1,
  Ring2,
  NEQUIPMENT
};


class Player : public Character {
public:
  explicit Player(bool give_equipment);

  Player(Player const&) = delete;
  Player& operator=(Player const&) = delete;

  static void save_player(std::ostream&);
  static void load_player(std::istream&);

  // Getters
  int get_ac() const override;
  bool has_true_sight() const override;
  bool can_sense_monsters() const;
  bool can_sense_magic() const;
  bool is_stealthy() const;
  int get_strength_with_bonuses() const;
  struct room* get_previous_room() const;

  // Modifier
  void increase_speed() override;
  void decrease_speed() override;
  void restore_strength() override;
  void modify_strength(int amount) override;
  void raise_level(int amount) override;

  // Setters
  void set_sense_magic();
  void remove_sense_magic();
  void set_sense_monsters();
  void remove_sense_monsters();
  void set_true_sight() override;
  void set_confused() override;
  void set_not_confused() override;
  void set_blind() override;
  void set_not_blind() override;
  void set_levitating() override;
  void set_not_levitating() override;
  void set_confusing_attack() override;
  void set_previous_room(struct room* room);

  // player_food.cc
  void         eat(Food*);
  void         digest_food();
  std::string  get_hunger_state() const;
  int          get_nutrition_left() const;
  static int   get_starting_nutrition();

  // player_pack_management.cc
  bool pack_show_inventory();
  bool pack_show_equipment();

  // player_pack.cc
  static size_t equipment_size();
  static size_t pack_size();
  bool          pack_add(Item* item, bool silent, bool from_floor);
  Item*         pack_remove(Item* item, bool create_new, bool all);
  Item*         pack_find_random_item();
  Item*         pack_find_item(int type, int subtype);
  class Weapon* pack_find_ammo(::Weapon::AmmoType);
  Item*         pack_find_item(std::string const& purpose, int subtype);
  class Weapon* equipped_weapon() const;
  class Armor*  equipped_armor() const;
  size_t        pack_num_items(int type, int subtype);
  bool          pack_contains_amulet();
  bool          pack_contains(Item const* item);
  size_t        pack_print_value();
  bool          pack_equip(Item *item, bool silent);
  bool          pack_unequip(Equipment pos, bool silent_on_success);
  void          give_gold(int amount);
  int           get_gold();
  void          pack_identify_item();
  void          equipment_run_abilities();
  int           equipment_food_drain_amount();
  void          pack_uncurse();
  bool          pack_swap_weapons();
  int           pack_get_ring_modifier(Ring::Type);

  // player_pack_management.cc
  enum Window {
    INVENTORY,
    EQUIPMENT
  };
  bool          pack_show_drop(Window mode);

  // Misc
  bool saving_throw(int which) const;
  bool has_ring_with_ability(int ability) const;
  void check_for_level_up();
  void waste_time(int rounds) const;
  void fall_asleep();
  void become_stuck();
  void become_poisoned();
  void teleport(Coordinate const* target); // random spot if target is nullptr
  bool can_see(Coordinate const& coord) const;
  bool can_see(Monster const& monster) const;
  void search();
  void rust_armor();
  std::string get_attack_string(bool successful_hit) const override;
  std::string get_name() const override;


private:
  struct room* previous_room;
  bool         senses_monsters;
  bool         senses_magic;

  static int constexpr darkvision = 2;
  static int constexpr lightvision = 4;

  // player_pack_management.cc
  bool pack_show(Window mode);
  bool pack_show_equip();
  bool pack_show_remove();

  // player_pack.cc
  static std::string  equipment_pos_to_string(Equipment pos);
  static std::vector<Equipment> all_rings();
  std::list<Item*>    pack;
  std::vector<Item*>  equipment;
  int                 gold;

  bool   pack_print_equipment();
  bool   pack_print_inventory(int subtype);

  // player_food.cc
  int          nutrition_left;

  static unsigned long long constexpr TAG_PLAYER          = 0x7000000000000000ULL;
  static unsigned long long constexpr TAG_INVENTORY       = 0x7000000000000001ULL;
  static unsigned long long constexpr TAG_EQUIPMENT       = 0x7000000000000002ULL;
  static unsigned long long constexpr TAG_SENSES_MONSTERS = 0x7000000000000003ULL;
  static unsigned long long constexpr TAG_SENSES_MAGIC    = 0x7000000000000004ULL;
  static unsigned long long constexpr TAG_GOLD            = 0x7000000000000005ULL;
  static unsigned long long constexpr TAG_NUTRITION       = 0x7000000000000006ULL;
};

extern Player* player;

/* Variable (TODO: remove these) */
extern int          player_turns_without_action; /* Turns asleep */
extern int          player_turns_without_moving; /* Turns held in place */
extern bool         player_alerted;              /* Alert the player? */
