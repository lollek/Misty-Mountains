#include <string>
#include <list>

#include "error_handling.h"
#include "coordinate.h"
#include "game.h"
#include "scrolls.h"
#include "io.h"
#include "armor.h"
#include "potions.h"
#include "rings.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "weapons.h"
#include "wand.h"
#include "things.h"
#include "monster.h"
#include "os.h"
#include "level_rooms.h"
#include "options.h"
#include "rogue.h"
#include "item.h"
#include "gold.h"

#include "pack.h"

using namespace std;

struct equipment_t {
  Item* ptr;
  string const description;
};

int const PACK_RENAMEABLE = -1;

int                         pack_gold = 0;
static list<Item*>*         player_pack = nullptr;
static vector<equipment_t>* equipment = nullptr;
static bool                 pack_used[26]; /* Is the character used in the pack? */

enum equipment_pos pack_ring_slots[PACK_RING_SLOTS] = {
  EQUIPMENT_RRING,
  EQUIPMENT_LRING
};

void init_pack() {
  player_pack = new list<Item*>;
  equipment = new vector<equipment_t> {
    { nullptr, "Body" },
    { nullptr, "Right Hand" },
    { nullptr, "Right Ring" },
    { nullptr, "Left Ring" }
  };
}

void free_pack() {
  delete player_pack;
  player_pack = nullptr;

  delete equipment;
  equipment = nullptr;
}

int
pack_size(void)
{
  return 22;
}

static size_t
pack_print_evaluate_item(Item* item)
{
  int worth = 0;
  if (item == nullptr)
    return 0;

  switch (item->o_type)
  {
    case FOOD:
      worth = 2 * item->o_count;
      break;

    case WEAPON: case AMMO: {
      Weapon* weapon = dynamic_cast<Weapon*>(item);
      if (weapon == nullptr) {
        error("Could not cast weapon to Weapon class");
      }
      worth = Weapon::worth(static_cast<Weapon::Type>(item->o_which));
      worth *= 3 * (item->get_hit_plus() + item->get_damage_plus()) + item->o_count;
      weapon->set_identified();
    } break;

    case ARMOR: {
      Armor* armor = dynamic_cast<Armor*>(item);
      if (armor == nullptr) {
        error("Could not cast armor to Armor class");
      }
      worth = Armor::value(static_cast<Armor::Type>(item->o_which));
      worth += (9 - item->get_armor()) * 100;
      worth += (10 * (Armor::ac(static_cast<Armor::Type>(item->o_which)) - item->get_armor()));
      armor->set_identified();
    } break;

    case SCROLL:
      {
        Scroll::Type scroll = static_cast<Scroll::Type>(item->o_which);
        worth = Scroll::worth(scroll);
        worth *= item->o_count;
        if (!Scroll::is_known(scroll))
          worth /= 2;
        Scroll::set_known(scroll);
      }
      break;

    case POTION: {
      Potion::Type subtype = static_cast<Potion::Type>(item->o_which);
      worth = Potion::worth(subtype);
      worth *= item->o_count;
      if (!Potion::is_known(subtype)) {
        worth /= 2;
      }
      Potion::set_known(subtype);
    } break;

    case RING: {
      Ring::Type subtype = static_cast<Ring::Type>(item->o_which);
      worth = Ring::worth(subtype);
      if (subtype == Ring::Type::ADDSTR || subtype == Ring::Type::ADDDAM ||
          subtype == Ring::Type::PROTECT || subtype == Ring::Type::ADDHIT) {
        if (item->get_armor() > 0) {
          worth += item->get_armor() * 100;
        } else {
          worth = 10;
        }
      }
      if (Ring::is_known(subtype)) {
        worth /= 2;
      }
      Ring::set_known(subtype);
    } break;

    case STICK: {
      Wand* wand = dynamic_cast<Wand* const>(item);
      if (wand == nullptr) {
        error("Could not cast wand to Wand class");
      }
      Wand::worth(static_cast<Wand::Type>(item->o_which));
      worth += 20 * wand->get_charges();
      if (!Wand::is_known(static_cast<Wand::Type>(item->o_which)))
        worth /= 2;
      Wand::set_known(static_cast<Wand::Type>(item->o_which));
    } break;

    case AMULET:
      worth = 1000;
      break;

    default:
      io_debug_fatal("Unknown type: %c(%d)", item->o_type, item->o_type);
      break;
  }

  if (worth < 0)
    worth = 0;

  printw("%5d  %s\n", worth, item->get_description().c_str());

  return static_cast<unsigned>(worth);
}

static char
pack_char(void)
{
  bool* bp;
  for (bp = pack_used; *bp; bp++)
    ;
  *bp = true;
  return static_cast<char>(bp - pack_used) + 'a';
}

void
pack_move_msg(Item* obj)
{
  Game::io->message("moved onto " + obj->get_description());
}

bool
pack_add(Item* obj, bool silent, bool from_floor)
{
  /* Either obj is an item or we try to take something from the floor */
  if (obj == nullptr)
  {
    obj = Game::level->get_item(player->get_position());
    if (obj == nullptr) {
      error("Item not found on floor");
    }
    from_floor = true;
  }

  /* Check for and deal with scare monster scrolls */
  if (obj->o_type == SCROLL && obj->o_which == Scroll::SCARE && obj->o_flags & ISFOUND)
  {
    Game::level->items.remove(obj);
    delete obj;
    Game::io->message("the scroll turns to dust as you pick it up");
    return false;
  }

  /* See if we can stack it with something else in the pack */
  bool is_picked_up = false;
  if (obj->o_type == POTION || obj->o_type == SCROLL || obj->o_type == FOOD
      || obj->o_type == AMMO)
    for (Item* ptr : *player_pack) {
      if (ptr->o_type == obj->o_type && ptr->o_which == obj->o_which &&
          ptr->get_hit_plus() == obj->get_hit_plus() &&
          ptr->get_damage_plus() == obj->get_damage_plus())
      {
        if (from_floor)
          Game::level->items.remove(obj);
        ptr->o_count += obj->o_count;
        ptr->set_position(obj->get_position());
        delete obj;
        obj = ptr;
        is_picked_up = true;
        break;
      }
    }

  /* If we cannot stack it, we need to have available space in the pack */
  if (!is_picked_up && pack_count_items() == pack_size())
  {
    Game::io->message("there's no room in your pack");
    if (from_floor)
      pack_move_msg(obj);
    return false;
  }

  /* Otherwise, just insert it */
  if (!is_picked_up)
  {
    if (from_floor)
      Game::level->items.remove(obj);
    player_pack->push_back(obj);
    obj->o_packch = pack_char();
  }

  obj->o_flags |= ISFOUND;


  /* Notify the user */
  if (!silent) {
    Game::io->message("you now have " + obj->get_description() +
                      " (" + string(1, obj->o_packch) + ")");
  }
  return true;
}

Item* pack_remove(Item* obj, bool newobj, bool all) {
  Item* return_value = obj;

  /* If there are several, we need to alloate a new item to hold it */
  if (obj->o_count > 1 && !all) {
    obj->o_count--;
    if (newobj) {
      return_value = obj->clone();
      return_value->o_count = 1;
    }

  /* Only one item? Just pop and return it */
  } else {
    pack_used[obj->o_packch - 'a'] = false;
    player_pack->remove(obj);
  }
  return return_value;
}


void pack_pick_up(Coordinate const& coord, bool force) {
  if (player->is_levitating()) {
    return;
  }

  // Collect all items which are at this location
  list<Item*> items_here;
  for (Item* item : Game::level->items) {
    if (item->get_position() == coord) {
      items_here.push_back(item);

      // If the item was of someone's desire, they will get mad and attack
      monster_aggro_all_which_desire_item(item);
    }
  }

  // No iterator in this loop, so we can delete while looping
  auto it = items_here.begin();
  while (it != items_here.end()) {
    Item* obj = *it;
    switch (obj->o_type) {

      case GOLD: {
        player->get_room()->r_goldval = 0;
        Gold* gold = dynamic_cast<Gold*>(obj);
        if (gold == nullptr) {
          error("casted gold to Gold* which became null");
        }

        int value = gold->get_amount();
        if (value > 0) {
          Game::io->message("you found " + to_string(value) + " gold pieces");
        }

        pack_gold += value;
        Game::level->items.remove(obj);

        delete obj;
        it = items_here.erase(it);
      } break;

      case POTION: case WEAPON: case AMMO: case FOOD: case ARMOR:
      case SCROLL: case AMULET: case RING: case STICK: {
        if (force || option_autopickup(obj->o_type)) {
          pack_add(obj, false, true);
          it = items_here.erase(it);
        } else {
          ++it;
        }
      } break;

      default: {
        error("Unknown type to pick up");
      }
    }
  }

  if (!items_here.empty()) {
    stringstream os;
    os << "items here: ";
    for (Item* item : items_here) {
      os << item->get_description();
      if (item != items_here.back()) {
        os << ", ";
      }
    }
    Game::io->message(os.str());
  }
}


Item*
pack_find_magic_item(void)
{
  int nobj = 0;

  for (Item* obj : *player_pack) {
    if (obj->is_magic() && os_rand_range(++nobj) == 0) {
      return obj;
    }
  }
  return nullptr;
}

Item*
pack_get_item(std::string const& purpose, int type)
{
  if (pack_count_items_of_type(type) < 1)
  {
    Game::io->message("You have no item to " + purpose);
    return nullptr;
  }

  pack_print_inventory(type);
  Game::io->message("which object do you want to " + purpose + "? ");
  char ch = io_readchar(true);
  Game::io->clear_message();

  pack_clear_inventory();

  if (ch == KEY_ESCAPE || ch == KEY_SPACE)
  {
    Game::io->clear_message();
    return nullptr;
  }

  for (Item* obj : *player_pack) {
    if (obj->o_packch == ch) {
      return obj;
    }
  }

  Game::io->message("'" + string(1, UNCTRL(ch)) + "s' is not a valid item");
  return nullptr;
}

bool
pack_is_empty(void)
{
  return player_pack->empty();
}

int
pack_count_items(void)
{
  return pack_count_items_of_type(0);
}

int
pack_count_items_of_type(int type)
{
  int num = 0;

  for (Item const* list : *player_pack) {
    if (!type || type == list->o_type ||
        (type == PACK_RENAMEABLE && (list->o_type != FOOD && list->o_type != AMULET))) {
      ++num;
    }
  }
  return num;
}

bool
pack_contains_amulet(void)
{
  return find_if(player_pack->cbegin(), player_pack->cend(),
      [] (Item const* ptr) {
    return ptr->o_type == AMULET;
  }) != player_pack->cend();
}

bool
pack_contains(Item *item)
{
  return find(player_pack->cbegin(), player_pack->cend(), item) != player_pack->cend();
}

bool
pack_print_equipment(void)
{
  WINDOW* equipscr = dupwin(stdscr);

  Coordinate orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  char sym = 'a';
  for (unsigned i = 0; i < NEQUIPMENT; ++i)
  {
    if (equipment->at(i).ptr != nullptr)
    {
      mvwprintw(equipscr, sym - 'a' + 1, 1, "%c) %s: %s",
                sym, equipment->at(i).description.c_str(),
                equipment->at(i).ptr->get_description().c_str());
      sym++;
    }
  }

  move(orig_pos.y, orig_pos.x);
  wrefresh(equipscr);
  delwin(equipscr);
  Game::io->message("--Press any key to continue--");
  io_readchar(false);
  touchwin(stdscr);
  Game::io->clear_message();
  return false;
}

bool
pack_print_inventory(int type)
{
  WINDOW* invscr = dupwin(stdscr);

  Coordinate orig_pos;
  getyx(stdscr, orig_pos.y, orig_pos.x);

  int num_items = 0;
  /* Print out all items */
  for (Item const* list : *player_pack) {
    if (!type || type == list->o_type ||
        (type == PACK_RENAMEABLE && (list->o_type != FOOD && list->o_type != AMULET))) {
      /* Print out the item and move to next row */
      wmove(invscr, ++num_items, 1);
      wprintw(invscr, "%c) %s", list->o_packch, list->get_description().c_str());
    }
  }

  wmove(stdscr, orig_pos.y, orig_pos.x);
  wrefresh(invscr);
  delwin(invscr);
  untouchwin(stdscr);
  return num_items != 0;
}

void
pack_clear_inventory(void)
{
  touchwin(stdscr);
}

size_t
pack_evaluate(void)
{
  size_t value = 0;

  clear();
  mvaddstr(0, 0, "Worth  Item  [Equipment]\n");
  for (int i = 0; i < NEQUIPMENT; ++i)
    value += pack_print_evaluate_item(pack_equipped_item(static_cast<equipment_pos>(i)));

  addstr("\nWorth  Item  [Inventory]\n");
  for (Item* obj : *player_pack) {
    value += pack_print_evaluate_item(obj);
  }

  printw("\n%5d  Gold Pieces          ", pack_gold);
  refresh();
  return value;
}

Item*
pack_equipped_item(enum equipment_pos pos)
{
  return equipment->at(pos).ptr;
}

bool
pack_equip_item(Item* item)
{
  enum equipment_pos pos;
  switch(item->o_type)
  {
    case ARMOR:
      pos = EQUIPMENT_ARMOR;
      break;

    case WEAPON: case AMMO:
      pos = EQUIPMENT_RHAND;
      break;

    case RING:
      pos = equipment->at(EQUIPMENT_RRING).ptr == nullptr
        ? EQUIPMENT_RRING
        : EQUIPMENT_LRING;
      break;

    default:
      pos = EQUIPMENT_RHAND;
      break;
  }

  if (equipment->at(pos).ptr)
    return false;
  else
  {
    equipment->at(pos).ptr = item;
    return true;
  }
}

bool
pack_unequip(enum equipment_pos pos, bool quiet_on_success)
{
  string const doing = pos == EQUIPMENT_RHAND
    ? "wielding"
    : "wearing";

  Item* obj = pack_equipped_item(pos);
  if (obj == nullptr)
  {
    Game::io->message("not " + doing + " anything!");
    return false;
  }

  if (obj->is_cursed())
  {
    Game::io->message("you can't. It appears to be cursed");
    return false;
  }

  equipment->at(pos).ptr = nullptr;

  /* Waste time if armor - since they take a while */
  if (pos == EQUIPMENT_ARMOR)
    player->waste_time(1);

  if (!pack_add(obj, true))
  {
    Game::level->items.push_back(obj);
    obj->set_position(player->get_position());
    Game::io->message("dropped " + obj->get_description());
  }
  else if (!quiet_on_success)
    Game::io->message("no longer " + doing + " " + obj->get_description());
  return true;
}


Item*
pack_find_arrow(void)
{
  auto results = find_if(player_pack->begin(), player_pack->end(),
      [] (Item *i) {
    return i->o_which == Weapon::ARROW;
  });

  return results == player_pack->end() ? nullptr : *results;
}

void
pack_identify_item(void)
{
  if (pack_is_empty())
  {
    Game::io->message("you don't have anything in your pack to identify");
    return;
  }

  Item* obj = pack_get_item("identify", 0);
  if (obj == nullptr)
    return;

  switch (obj->o_type)
  {
    case SCROLL: Scroll::set_known(static_cast<Scroll::Type>(obj->o_which)); break;
    case POTION: Potion::set_known(static_cast<Potion::Type>(obj->o_which)); break;
    case STICK:  Wand::set_known(static_cast<Wand::Type>(obj->o_which)); break;
    case RING:   Ring::set_known(static_cast<Ring::Type>(obj->o_which)); break;
    case WEAPON: {
      Weapon* weapon = dynamic_cast<Weapon*>(obj);
      if (weapon == nullptr) {
        error("Could not cast weapon to Weapon class");
      }
      weapon->set_identified();
    } break;
    case ARMOR: {
      Armor* armor = dynamic_cast<Armor*>(obj);
      if (armor == nullptr) {
        error("Could not cast armor to Armor class");
      }
      armor->set_identified();
    } break;
    default: break;
  }

  Game::io->message(obj->get_description());
}


