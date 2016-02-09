#include <string>
#include <sstream>

#include "error_handling.h"
#include "game.h"
#include "command.h"
#include "io.h"
#include "player.h"
#include "rings.h"
#include "os.h"
#include "death.h"

#include "food.h"

using namespace std;

static Food::Type random_food_type() {
  return os_rand_range(10) ? Food::IronRation : Food::Fruit;
}

Food::~Food() {}

Food* Food::clone() const {
  return new Food(*this);
}

bool Food::is_magic() const {
  return false;
}

Food::Food() : Food(random_food_type()) {}

Food::Food(Food::Type subtype_) : Item(), subtype(subtype_) {

  // Reset global food counter
  Game::levels_without_food = 0;

  o_count = 1;
  o_type = IO::Food;
  o_which = subtype;
}

Food::Type Food::get_type() const {
  return subtype;
}

string Food::get_description() const {
  stringstream os;

  string type;
  switch (subtype) {
    case Fruit:      type = "fruit"; break;
    case IronRation: type = "food ration"; break;
    case NFOODS: error("Unknown food type NFOODS");
  }

  if (o_count == 1) {
    os << "a " << type;
  } else {
    os << to_string(o_count) << " " << type << "s";
  }

  return os.str();
}

void Food::set_identified() {
}

bool Food::is_identified() const {
  return true;
}
