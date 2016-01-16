#pragma once

#include <vector>
#include <string>

#include "item.h"
#include "things.h"

enum weapon_type {
  MACE     = 0,
  SWORD    = 1,
  BOW      = 2,
  ARROW    = 3,
  DAGGER   = 4,
  TWOSWORD = 5,
  DART     = 6,
  SHIRAKEN = 7,
  SPEAR    = 8,
  MAXWEAPONS
};
#define FLAME MAXWEAPONS /* fake entry for dragon breath (ick) */

extern std::vector<obj_info> weapon_info;

/* Drop an item someplace around here. */
void weapon_missile_fall(Item* obj, bool pr);

Item* weapon_create(int which, bool random_stats);

bool weapon_wield(Item* weapon);
bool weapon_wield_last_used();
void weapon_set_last_used(Item* weapon);

std::string weapon_description(Item const* item);
