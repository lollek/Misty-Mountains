#include <string>
#include <vector>

#include "game.h"
#include "error_handling.h"
#include "coordinate.h"
#include "io.h"
#include "misc.h"
#include "player.h"

#include "options.h"

using namespace std;

bool fight_flush  = false;
bool jump         = true;
bool passgo       = false;
bool use_colors   = true;

static bool pickup_potions = true;
static bool pickup_scrolls = true;
static bool pickup_food    = true;
static bool pickup_weapons = false;
static bool pickup_armor   = false;
static bool pickup_rings   = true;
static bool pickup_sticks  = true;
static bool pickup_ammo    = true;

bool option_autopickup(int type) {
  switch (type) {
    case IO::Ammo:   return pickup_ammo;
    case IO::Potion: return pickup_potions;
    case IO::Scroll: return pickup_scrolls;
    case IO::Food:   return pickup_food;
    case IO::Weapon: return pickup_weapons;
    case IO::Armor:  return pickup_armor;
    case IO::Ring:   return pickup_rings;
    case IO::Wand:   return pickup_sticks;
    case IO::Amulet: return true;
    default:     error("option_autopickup: unknown type: " + to_string(type));
  }
}

bool option() {

  struct option {
    char index;            // What to press to change option
    string const o_prompt; // prompt for interactive entry
    void* o_opt;           // pointer to thing to set function to print value
    enum put_t { BOOL, STR } put_type;
  };

  vector<option> optlist {
    {'1',        "Flush typeahead during battle?....", &fight_flush,    option::BOOL},
    {'2',        "Show position only at end of run?.", &jump,           option::BOOL},
    {'3',        "Follow turnings in passageways?...", &passgo,         option::BOOL},
    {IO::Potion, "Pick up potions?..................", &pickup_potions, option::BOOL},
    {IO::Scroll, "Pick up scrolls?..................", &pickup_scrolls, option::BOOL},
    {IO::Food,   "Pick up food?.....................", &pickup_food,    option::BOOL},
    {IO::Weapon, "Pick up weapons?..................", &pickup_weapons, option::BOOL},
    {IO::Armor,  "Pick up armor?....................", &pickup_armor,   option::BOOL},
    {IO::Ring,   "Pick up rings?....................", &pickup_rings,   option::BOOL},
    {IO::Wand,   "Pick up sticks?...................", &pickup_sticks,  option::BOOL},
    {IO::Ammo,   "Pick up ammo?.....................", &pickup_ammo,    option::BOOL},
    {'4',        "Name..............................", Game::whoami,    option::STR},
  };

  string const query = "Which value do you want to change? (ESC to exit) ";
  Coordinate const msg_pos (static_cast<int>(query.size()), 0);
  Game::io->message(query);

  WINDOW* optscr = dupwin(stdscr);

  // Display current values of options
  wmove(optscr, 1, 0);
  for (size_t i = 0; i < optlist.size(); ++i) {

    wprintw(optscr, "%c) %s", optlist.at(i).index, optlist.at(i).o_prompt.c_str());
    switch (optlist.at(i).put_type) {
      case option::BOOL: {
        waddstr(optscr, *static_cast<bool*>(optlist.at(i).o_opt) ? "True" : "False");
      } break;

      case option::STR: {
        waddstr(optscr, static_cast<string*>(optlist.at(i).o_opt)->c_str());
      } break;
    }
    waddch(optscr, '\n');
  }

  // Loop and change values until user presses escape
  char c = static_cast<char>(~KEY_ESCAPE);
  while (c != KEY_ESCAPE) {

    wmove(optscr, msg_pos.y, msg_pos.x);
    wrefresh(optscr);
    c = io_readchar(true);

    auto change_option = find_if(optlist.begin(), optlist.end(),
        [c] (option const& o) {
      return o.index == c;
    });

    if (change_option != optlist.end()) {
      int i = static_cast<int>(change_option - optlist.begin());
      option const& opt = *change_option;
      wmove(optscr, i + 1, 3 + static_cast<int>(opt.o_prompt.size()));
      switch (opt.put_type) {
        case option::BOOL: {
          bool* b = static_cast<bool*>(opt.o_opt);
          *b = !*b;
          waddstr(optscr, *b ? "True " : "False");
          wrefresh(optscr);
        } break;

        case option::STR: {
          string* str = static_cast<string*>(opt.o_opt);
          *str = Game::io->read_string(optscr, str);
        } break;
      }
    }
  }

  /* Switch back to original screen */
  wmove(optscr, LINES - 1, 0);
  delwin(optscr);
  clearok(curscr, true);
  touchwin(stdscr);
  Game::io->clear_message();
  return false;
}

