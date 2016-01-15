#pragma once

#include <string>

#include "damage.h"
#include "Coordinate.h"

#define o_charges	o_arm
#define o_goldval	o_arm

/* flags for objects */
#define ISCURSED 000001		/* object is cursed */
#define ISKNOW	0000002		/* player knows details about the object */
#define ISMISL	0000004		/* object is a missile type */
#define ISMANY	0000010		/* object comes in groups */
#ifndef ISFOUND
#define ISFOUND 0000020		/*...is used for both objects and creatures */
#endif /* ISFOUND */
#define ISPROT	0000040		/* armor is permanently protected */


struct item {
  Coordinate    o_pos;                 /* Where it lives on the screen */
  std::string   o_label;               /* Label for object */
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
};

/* Data */
static inline Coordinate* item_pos(item* item)
{ return &item->o_pos; }
static inline std::string const& item_nickname(item const* item)
{ return item->o_label; }
static inline int item_type(item const* item)
{ return item->o_type; }
static inline int item_subtype(item const* item)
{ return item->o_which; }
static inline int item_count(item const* item)
{ return item->o_count; }
static inline int item_charges(item const* item)
{ return item->o_charges; }
static inline int item_armor(item const* item)
{ return item->o_arm; }
static inline struct damage const* item_throw_damage(item const* item)
{ return &item->o_hurldmg; }
static inline struct damage const* item_damage(item const* item)
{ return &item->o_damage; }
static inline int item_bonus_hit(item const* item)
{ return item->o_hplus; }
static inline int item_bonus_damage(item const* item)
{ return item->o_dplus; }

/* Flags */
static inline bool item_is_known(item const* item)
{ return item->o_flags & ISKNOW; }
