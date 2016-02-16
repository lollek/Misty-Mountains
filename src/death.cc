#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>

#include <string>
#include <sstream>

#include "game.h"
#include "io.h"
#include "misc.h"
#include "monster.h"
#include "score.h"

#include "death.h"

using namespace std;

static void death(int type) __attribute__((noreturn));

string death_reason(int reason) {
  if (reason < 256) {
    string monster = Monster::name(static_cast<Monster::Type>(reason));
    stringstream os;
    os
      << "Killed by a"
      << vowelstr(monster)
      << " "
      << monster;
    return os.str();

  } else {
    switch (static_cast<enum death_reason>(reason)) {
      case DEATH_UNKNOWN:    return "Died by an unknown cause";
      case DEATH_ARROW:      return "Pierced by an arrow";
      case DEATH_BOLT:       return "Pierced by a bolt";
      case DEATH_DART:       return "Poisoned by a dart";
      case DEATH_FLAME:      return "Burned to crisp";
      case DEATH_ICE:        return "Incased in ice";
      case DEATH_HUNGER:     return "Starved to death";
      case DEATH_NO_HEALTH:  return "Reduced to a lifeless shell";
      case DEATH_NO_EXP:     return "Got their soul drained";
    }
  }
}

static void death(int type) {
  player->give_gold(-player->get_gold() / 10);

  Game::io->refresh();
  Game::io->message("You die!");
  Game::io->readchar(false);

  player->pack_print_value();
  score_show_and_exit(player->get_gold(), player->pack_contains_amulet() ? 3 : 0, type);
}

void death(enum death_reason reason) {
  death(static_cast<int>(reason));
}

void death(Monster::Type reason) {
  death(static_cast<int>(reason));
}
