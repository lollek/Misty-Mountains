#include <vector>
#include <sstream>

#include "item/food.h"
#include "item/gold.h"
#include "error_handling.h"
#include "game.h"
#include "coordinate.h"
#include "item/potions.h"
#include "item/scrolls.h"
#include "command.h"
#include "options.h"
#include "io.h"
#include "item/armor.h"
#include "item/rings.h"
#include "misc.h"
#include "level.h"
#include "item/weapons.h"
#include "player.h"
#include "item/wand.h"
#include "traps.h"
#include "os.h"
#include "rogue.h"

#include "wizard.h"

using namespace std;

bool wizard = false;
bool wizard_dicerolls = false;

static void
pr_spec(char type)
{
  size_t max;
  switch (type) {
    case IO::Potion: max = Potion::Type::NPOTIONS;  break;
    case IO::Scroll: max = Scroll::Type::NSCROLLS;  break;
    case IO::Ring:   max = Ring::Type::NRINGS;    break;
    case IO::Wand:   max = Wand::Type::NWANDS; break;
    case IO::Armor:  max = Armor::Type::NARMORS;   break;
    case IO::Weapon: max = Weapon::NWEAPONS; break;
    case IO::Trap:   max = Trap::NTRAPS; break;

    default:
      Game::io->message("Unknown type in pr_spec"); return;
  }

  char ch = '0';
  for (size_t i = 0; i < max; ++i) {
    string name;
    Game::io->move_pointer(1, static_cast<int>(i) + 1);

    switch (type) {
      case IO::Scroll: name = Scroll::name(static_cast<Scroll::Type>(i)); break;
      case IO::Armor:  name = Armor::name(static_cast<Armor::Type>(i)); break;
      case IO::Potion: name = Potion::name(static_cast<Potion::Type>(i)); break;
      case IO::Wand:   name = Wand::name(static_cast<Wand::Type>(i)); break;
      case IO::Ring:   name = Ring::name(static_cast<Ring::Type>(i)); break;
      case IO::Weapon: name = Weapon::name(static_cast<Weapon::Type>(i)); break;
      case IO::Trap:   name = Trap::name(static_cast<Trap::Type>(i)); break;
      default: error("Unknown type in pr_spec");
    }

    stringstream ss;
    ss << ch << ": " << name;
    Game::io->print_string(ss.str());
    ch = ch == '9' ? 'a' : (ch + 1);
  }

  Game::io->force_redraw();
  Game::io->readchar(false);
  Game::io->clear_message();
  Game::io->clear_screen();
}

int
wizard_list_items(void)
{
  Game::io->message("for what type of object do you want a list?");
  vector<char> things {
    IO::Potion, IO::Scroll, IO::Food, IO::Weapon, IO::Armor, IO::Ring, IO::Wand,
    IO::Trap
  };
  vector<string> names {
    "potion", "scroll", "food", "weapon", "armor", "ring", "wand", "trap",
  };

  for (size_t i = 0; i < things.size(); ++i) {
    stringstream ss;
    ss << things.at(i) << " " << names.at(i);
    Game::io->print_string(1, static_cast<int>(i) + 1, ss.str());
  }

  char ch = static_cast<char>(Game::io->readchar(true));
  Game::io->force_redraw();

  pr_spec(ch);
  return 0;
}

void wizard_create_item(void) {

  // Get itemtype
  Game::io->message("type of item?");
  char type = static_cast<char>(Game::io->readchar(true));
  Game::io->clear_message();

  switch (type) {

    // Supported types:
    case IO::Weapon: case IO::Armor: case IO::Ring: case IO::Wand:
    case IO::Gold: case IO::Potion: case IO::Scroll: case IO::Trap: case IO::Food: {
      break;
    }

    default: {
      Game::io->message("Bad pick");
      return;
    }
  }

  // Get item subtype
  Game::io->message("which " + string(1, type) + " do you want? (0-f)");
  char ch = Game::io->readchar(true);
  int which = isdigit(ch) ? ch - '0' : ch - 'a' + 10;
  Game::io->clear_message();

  // Allocate item
  Item* obj = nullptr;
  switch (type) {
    case IO::Trap: {
      if (which < 0 || which >= Trap::NTRAPS) {
        Game::io->message("Bad trap id");

      } else {
        Game::level->set_not_real(player->get_position());
        Game::level->set_trap_type(player->get_position(), static_cast<Trap::Type>(which));
      }
    } return;

    case IO::Wand: {
      obj = new Wand(static_cast<Wand::Type>(which));
    } break;

    case IO::Scroll: {
      obj = new Scroll(static_cast<Scroll::Type>(which));
      obj->o_count = 10;
    } break;

    case IO::Potion: {
      obj = new Potion(static_cast<Potion::Type>(which));
      obj->o_count = 10;
    } break;

    case IO::Food: {
      obj = new Food(static_cast<Food::Type>(which));
    } break;

    case IO::Weapon: {
      obj = new class Weapon(static_cast<Weapon::Type>(which), false);

      Game::io->message("blessing? (+,-,n)");
      char bless = Game::io->readchar(true);
      Game::io->clear_message();

      if (bless == '-') {
        obj->set_cursed();
        obj->modify_hit_plus(-os_rand_range(3) + 1);

      } else if (bless == '+')
        obj->modify_hit_plus(os_rand_range(3) + 1);
    } break;

    case IO::Armor: {
      obj = new class Armor(false);

      Game::io->message("blessing? (+,-,n)");
      char bless = Game::io->readchar(true);
      Game::io->clear_message();
      if (bless == '-') {
        obj->set_cursed();
        obj->modify_armor(os_rand_range(3) + 1);

      } else if (bless == '+')
        obj->modify_armor(-os_rand_range(3) + 1);
    } break;

    case IO::Ring: {
      obj = new Ring(static_cast<Ring::Type>(which));
    } break;

    case IO::Gold: {
      Game::io->message("how much?");
      int amount = stoi(Game::io->read_string());
      obj = new Gold(amount);
    } break;

    default: {
      error("Unimplemented item: " + to_string(which));
    }
  }

  if (obj == nullptr) {
    error("object was null");
  }
  player->pack_add(obj, false, false);
}

void wizard_show_map(void) {
  for (int y = 0; y < IO::map_height; y++)  {
    for (int x = 0; x < IO::map_width; x++) {
      long unsigned int ch = 0;

      Monster* monster = Game::level->get_monster(x, y);
      if (ch == 0 && monster != nullptr) {
        ch = static_cast<long unsigned int>(monster->get_look());
      }

      Item* item = Game::level->get_item(x, y);
      if (ch == 0 && item != nullptr) {
        ch = static_cast<long unsigned int>(item->get_item_type());
      }

      if (ch == 0) {
        Tile::Type tile = Game::level->get_tile(x, y);
        switch (tile) {
          case Tile::Floor:        ch = IO::Floor; break;
          case Tile::Wall:         ch = IO::Wall; break;
          case Tile::OpenDoor:     ch = IO::OpenDoor; break;
          case Tile::ClosedDoor:   ch = IO::ClosedDoor; break;
          case Tile::Trap:         ch = IO::Trap; break;
          case Tile::StairsDown:   ch = IO::StairsDown; break;
          case Tile::StairsUp:     ch = IO::StairsUp; break;
          case Tile::Shop:         ch = IO::Shop; break;
        }
      }

      IO::Attribute attr = IO::None;
      if (!Game::level->is_real(x, y)) {
        attr = IO::Attribute::Standout;
      }

      Game::io->print(x, y, ch, attr);
    }
  }

  Game::io->force_redraw();
  Game::io->readchar(false);
  Game::io->clear_screen();
}

void wizard_levels_and_gear(void) {
  player->raise_level(9);

  /* Give him a sword (+1,+1) */
  class Weapon* weapon = new class Weapon(Weapon::Claymore, false);
  weapon->set_hit_plus(1);
  weapon->set_damage_plus(1);
  weapon->set_identified();
  if (!player->pack_equip(weapon, false)) {
    delete weapon;
  }

  /* And his suit of armor */
  class Armor* armor = new class Armor(Armor::Type::Mithrilchainmail, false);
  armor->modify_armor(5);
  armor->set_identified();
  if (!player->pack_equip(armor, false)) {
    delete armor;
  }
}
