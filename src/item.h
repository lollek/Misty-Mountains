#pragma once

#include <string>
#include <istream>
#include <ostream>

#include "damage.h"
#include "coordinate.h"

class Item {
public:
  enum Type {
    Potion,
    Scroll,
    Food,
    Weapon,
    Armor,
    Ring,
    Wand,
    Amulet,
    Gold,
    NITEMS
  };

  explicit Item(Item const&) = default;

  virtual ~Item();

  Item& operator=(Item const&) = default;
  Item& operator=(Item&&) = default;
  virtual Item* clone() const = 0;

  // Setters (virtual)
  virtual void    set_identified() = 0;

  // Setters
  virtual void set_position(Coordinate const&);
  virtual void set_x(int);
  virtual void set_y(int);
  virtual void set_nickname(std::string const&);
  virtual void set_attack_damage(damage const&);
  virtual void set_throw_damage(damage const&);
  virtual void set_hit_plus(int value);
  virtual void set_damage_plus(int value);
  virtual void set_armor(int value);
  virtual void set_cursed();
  virtual void set_not_cursed();

  // Modifiers
  virtual void modify_hit_plus(int amount);
  virtual void modify_damage_plus(int amount);
  virtual void modify_armor(int amount);

  // Getters (virtual)
  virtual std::string   get_description() const = 0;
  virtual bool          is_magic() const = 0;
  virtual bool          is_identified() const = 0;
  virtual bool          is_stackable() const = 0;
  virtual bool          autopickup() const = 0;
  virtual int           get_value() const = 0;
  virtual int           get_base_value() const = 0;

  // Getters
  virtual Coordinate const&     get_position() const;
  virtual int                   get_x() const;
  virtual int                   get_y() const;
  virtual std::string const&    get_nickname() const;
  virtual int                   get_item_type() const;
  virtual damage const&         get_attack_damage() const;
  virtual damage const&         get_throw_damage() const;
  virtual int                   get_hit_plus() const;
  virtual int                   get_damage_plus() const;
  virtual int                   get_armor() const;
  virtual bool                  is_cursed() const;

  int           o_type;                // What kind of object it is
  int           o_launch;              // What you need to launch it
  int           o_count;               // count for plural objects
  int           o_which;               // Which object of a type it is
  int           o_flags;               // information about objects
  char          o_packch;              // What character it is in the pack

  virtual void          save(std::ostream&) const;
  virtual bool          load(std::istream&);

  // Static
  static int         probability(Type type);
  static std::string name(Type type);
  static Item*       random();

protected:
  Item();

private:
  Coordinate    position_on_screen;
  std::string   nickname;              // TODO: See if we can remove this
  damage        attack_damage;         // Damage if used like sword
  damage        throw_damage;          // Damage if thrown
  int           hit_plus;              // Plusses to hit
  int           damage_plus;           // Plusses to damage
  int           armor;                 // Armor protection
  bool          cursed;


  static unsigned long long constexpr TAG_ITEM            = 0x9000000000000000ULL;
  static unsigned long long constexpr TAG_ITEM_PUBLIC     = 0x9000000000000001ULL;
  static unsigned long long constexpr TAG_ITEM_PRIVATE    = 0x9000000000000002ULL;
};

