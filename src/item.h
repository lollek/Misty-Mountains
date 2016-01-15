#pragma once

#include <string>

#include "things.h"

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
