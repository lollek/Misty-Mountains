#include <string>
#include <sstream>

#include "disk.h"
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
  vector<Food::Type> potential_food;

  switch (Game::current_level) {
    default:

    case 1:
      potential_food.push_back(Food::Fruit);
      potential_food.push_back(Food::IronRation);
  }

  return potential_food.at(os_rand_range(potential_food.size()));
}

Food::~Food() {}

Food* Food::clone() const {
  return new Food(*this);
}

bool Food::is_magic() const {
  return false;
}


Food::Food(std::ifstream& data) {
  load(data);
}

Food::Food() : Food(random_food_type()) {}

Food::Food(Food::Type subtype_) : Item(), subtype(subtype_) {

  food_value = food_for_type(subtype);

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

int Food::get_nutrition_value() const {
  return food_value;
}

void Food::save(std::ofstream& data) const {
  Item::save(data);
  static_assert(sizeof(Food::Type) == sizeof(int), "Wrong Food::Type size");
  Disk::save(TAG_FOOD, static_cast<int>(subtype), data);
  Disk::save(TAG_FOOD, food_value, data);
}

bool Food::load(std::ifstream& data) {
  if (!Item::load(data) ||
      !Disk::load(TAG_FOOD, reinterpret_cast<int&>(subtype), data) ||
      !Disk::load(TAG_FOOD, food_value, data)) {
    return false;
  }
  return true;
}


int Food::food_for_type(Type subtype) {
  switch (subtype) {
    case Food::Fruit:        return 1000;
    case Food::IronRation:   return 3500;

    case NFOODS: error("Bad value NFOODS");
  }
}
