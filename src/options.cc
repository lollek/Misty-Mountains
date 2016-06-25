#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "coordinate.h"
#include "error_handling.h"
#include "game.h"
#include "io.h"
#include "misc.h"
#include "player.h"

#include "options.h"

using namespace std;

bool fight_flush{false};
bool jump{true};
bool passgo{false};
bool use_colors{true};

static bool pickup_potions{true};
static bool pickup_scrolls{true};
static bool pickup_food{true};
static bool pickup_weapons{false};
static bool pickup_armor{false};
static bool pickup_rings{true};
static bool pickup_sticks{true};
static bool pickup_ammo{true};

bool option_autopickup(int type) {
  switch (type) {
    case IO::Ammo: return pickup_ammo;
    case IO::Potion: return pickup_potions;
    case IO::Scroll: return pickup_scrolls;
    case IO::Food: return pickup_food;
    case IO::Weapon: return pickup_weapons;
    case IO::Armor: return pickup_armor;
    case IO::Ring: return pickup_rings;
    case IO::Wand: return pickup_sticks;
    default: error("option_autopickup: unknown type: " + to_string(type));
  }
}

bool option() {
  struct option {
    char index;             // What to press to change option
    string const o_prompt;  // prompt for interactive entry
    void* o_opt;            // pointer to thing to set function to print value
    enum put_t { BOOL, STR } put_type;
  };

  vector<option> optlist{
      {'1', "Flush typeahead during battle?....", &fight_flush, option::BOOL},
      {'2', "Show position only at end of run?.", &jump, option::BOOL},
      {'3', "Follow turnings in passageways?...", &passgo, option::BOOL},
      {IO::Potion, "Pick up potions?..................", &pickup_potions,
       option::BOOL},
      {IO::Scroll, "Pick up scrolls?..................", &pickup_scrolls,
       option::BOOL},
      {IO::Food, "Pick up food?.....................", &pickup_food,
       option::BOOL},
      {IO::Weapon, "Pick up weapons?..................", &pickup_weapons,
       option::BOOL},
      {IO::Armor, "Pick up armor?....................", &pickup_armor,
       option::BOOL},
      {IO::Ring, "Pick up rings?....................", &pickup_rings,
       option::BOOL},
      {IO::Wand, "Pick up sticks?...................", &pickup_sticks,
       option::BOOL},
      {IO::Ammo, "Pick up ammo?.....................", &pickup_ammo,
       option::BOOL},
      {'4', "Name..............................", Game::whoami, option::STR},
  };

  string const query{"Which value do you want to change? (ESC to exit) "};
  Coordinate const msg_pos(static_cast<int>(query.size()), 0);

  // Display current values of options
  Game::io->move_pointer(0, 1);
  for (size_t i{0}; i < optlist.size(); ++i) {
    int x{0};
    int y{1 + static_cast<int>(i)};

    stringstream os;
    os << optlist.at(i).index << ") " << optlist.at(i).o_prompt;
    string option_string = os.str();

    Game::io->print_string(IO::message_x + x, y, option_string);
    switch (optlist.at(i).put_type) {
      case option::BOOL: {
        Game::io->print_string(
            *static_cast<bool*>(optlist.at(i).o_opt) ? "True" : "False");
      } break;

      case option::STR: {
        Game::io->print_string(*static_cast<string*>(optlist.at(i).o_opt));
      } break;
    }
  }

  // Loop and change values until user presses escape
  char c{static_cast<char>(~KEY_ESCAPE)};
  while (c != KEY_ESCAPE) {
    Game::io->clear_message();
    Game::io->message(query);

    Game::io->move_pointer(msg_pos.x, msg_pos.y);
    Game::io->force_redraw();
    c = Game::io->readchar(true);

    auto const change_option{
        find_if(optlist.begin(), optlist.end(),
                [c](option const& o) { return o.index == c; })};

    if (change_option != optlist.end()) {
      int i{static_cast<int>(change_option - optlist.begin())};
      option const& opt{*change_option};
      switch (opt.put_type) {
        case option::BOOL: {
          bool* b{static_cast<bool*>(opt.o_opt)};
          *b = !*b;
          Game::io->print_string(
              IO::message_x + 3 + static_cast<int>(opt.o_prompt.size()), i + 1,
              *b ? "True " : "False");
          Game::io->force_redraw();
        } break;

        case option::STR: {
          Game::io->move_pointer(3 + static_cast<int>(opt.o_prompt.size()), i);
          string* str = static_cast<string*>(opt.o_opt);
          *str = Game::io->read_string(str, false);
        } break;
      }
    }
  }

  /* Switch back to original screen */
  Game::io->clear_message();
  return false;
}
