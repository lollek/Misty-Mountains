#pragma once

#include <string>

#include "damage.h"
#include "Coordinate.h"

#define o_charges	o_arm

/* flags for objects */
#define ISCURSED 000001		/* object is cursed */
#define ISKNOW	0000002		/* player knows details about the object */
#define ISMISL	0000004		/* object is a missile type */
#define ISMANY	0000010		/* object comes in groups */
#ifndef ISFOUND
#define ISFOUND 0000020		/*...is used for both objects and creatures */
#endif /* ISFOUND */
#define ISPROT	0000040		/* armor is permanently protected */


class Item {
public:
  Item() = default;
  Item(Item const&) = default;

  virtual ~Item();

  Item& operator=(Item const&) = default;
  Item& operator=(Item&&) = default;
  virtual Item* clone() const = 0;

  /* Setters */
  void set_position(Coordinate const&);
  void set_x(int);
  void set_y(int);
  void set_nickname(std::string const&);

  /* Getters */
  virtual std::string   get_description() const = 0;
  virtual bool          is_magic() const = 0;
  Coordinate const&     get_position() const;
  int                   get_x() const;
  int                   get_y() const;
  std::string const&    get_nickname() const;
  int                   get_type() const;

  int           o_type;                /* What kind of object it is */
  int           o_launch;              /* What you need to launch it */
  int           o_count;               /* count for plural objects */
  int           o_which;               /* Which object of a type it is */
  int           o_hplus;               /* Plusses to hit */
  int           o_dplus;               /* Plusses to damage */
  int           o_arm;                 /* Armor protection */
  int           o_flags;               /* information about objects */
  char          o_packch;              /* What character it is in the pack */
  damage        o_damage;              /* Damage if used like sword */
  damage        o_hurldmg;             /* Damage if thrown */

private:
  Coordinate    position_on_screen;
  std::string   nickname;
};

static inline int item_type(Item const* item)
{ return item->o_type; }
static inline int item_subtype(Item const* item)
{ return item->o_which; }
static inline int item_count(Item const* item)
{ return item->o_count; }
static inline int item_charges(Item const* item)
{ return item->o_charges; }
static inline int item_armor(Item const* item)
{ return item->o_arm; }
static inline struct damage const* item_throw_damage(Item const* item)
{ return &item->o_hurldmg; }
static inline struct damage const* item_damage(Item const* item)
{ return &item->o_damage; }
static inline int item_bonus_hit(Item const* item)
{ return item->o_hplus; }
static inline int item_bonus_damage(Item const* item)
{ return item->o_dplus; }

/* Flags */
static inline bool item_is_known(Item const* item)
{ return item->o_flags & ISKNOW; }
