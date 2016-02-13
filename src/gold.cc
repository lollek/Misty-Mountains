#include <string>

#include "error_handling.h"
#include "os.h"
#include "game.h"

#include "gold.h"

using namespace std;

Gold::~Gold() {}

Gold::Gold() : Gold(random_gold_amount()) {}

Gold::Gold(int amount_) : Item(), amount(amount_) {
  o_type = IO::Gold;
}

int Gold::get_amount() const {
  return amount;
}

int Gold::random_gold_amount() {
  return os_rand_range(50 + 10 * Game::current_level) + 2;
}

string Gold::get_description() const {
  return to_string(amount) + " gold pieces";
}

Gold* Gold::clone() const {
  return new Gold(*this);
}

bool Gold::is_magic() const {
  return false;
}

void Gold::set_identified() {
}

bool Gold::is_identified() const {
  return true;
}
