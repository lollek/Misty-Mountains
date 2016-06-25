#pragma once

#include <string>

#include "monster.h"

// Reasons for player dying
enum death_reason {
  DEATH_UNKNOWN = 256,
  DEATH_ARROW = 257,
  DEATH_BOLT = 258,
  DEATH_DART = 259,
  DEATH_FLAME = 260,
  DEATH_ICE = 261,
  DEATH_HUNGER = 262,
  DEATH_NO_HEALTH = 263,
  DEATH_NO_EXP = 264,

  QUIT = 265,
  WON = 266,
};

// Return a string describing the death
std::string death_reason(int reason);

// Handle player death
void death(enum death_reason reason) __attribute__((noreturn));
void death(Monster::Type reason) __attribute__((noreturn));
