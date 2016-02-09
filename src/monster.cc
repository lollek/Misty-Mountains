#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <string>
#include <vector>
#include <sstream>

#include "gold.h"
#include "magic.h"
#include "command.h"
#include "daemons.h"
#include "error_handling.h"
#include "game.h"
#include "io.h"
#include "scrolls.h"
#include "rings.h"
#include "level_rooms.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "weapons.h"
#include "os.h"
#include "armor.h"
#include "options.h"
#include "rogue.h"
#include "death.h"

#include "monster.h"

using namespace std;

int            monster_flytrap_hit = 0; // Number of time flytrap has hit

vector<monster_template> const* Monster::monsters = nullptr;

void Monster::init_monsters() {
//      010000000000000000000: greedy
//      020000000000000000000: mean
//      040000000000000000000: flying
//      001000000000000000000: regen
//      002000000000000000000: invis
  monsters = new vector<monster_template> const {
//CARRY  FLAG                   speed,  exp,lvl, amr, dmg              */
    { "aquator",
     0,  020000000000000000000ULL,  1,   20,  5,  18, {{0,1}}},
    { "bat",
     0,  040000000000000000000ULL,  1,    1,  1,  17, {{1,2}}},
    { "centaur",
    15,  000000000000000000000ULL,  1,   17,  4,  16, {{1,2},{1,5},{1,5}}},
    { "dragon",
   100,  020000000000000000000ULL,  2, 5000, 10,  21, {{1,8},{1,8},{3,10}}},
    { "emu",
     0,  020000000000000000000ULL,  1,    2,  1,  13, {{1,2}}},
    { "venus flytrap",
     0,  020000000000000000000ULL,  1,   80,  8,  17, {{0,1}}},
    { "griffin",
    20,  061000000000000000000ULL,  2, 2000, 13,  18, {{4,3},{3,5}}},
    { "hobgoblin",
     0,  020000000000000000000ULL,  1,    3,  1,  15, {{1,8}}},
    { "ice monster",
     0,  000000000000000000000ULL,  1,    5,  1,  11, {{0,1}}},
    { "jabberwock",
    70,  000000000000000000000ULL,  1, 3000, 15,  14, {{2,12},{2,4}}},
    { "kestrel",
     0,  060000000000000000000ULL,  2,    1,  1,  13, {{1,4}}},
    { "leprechaun",
     0,  000000000000000000000ULL,  1,   10,  3,  12, {{1,1}}},
    { "medusa",
    40,  020000000000000000000ULL,  1,  200,  8,  18, {{3,4},{3,4},{2,5}}},
    { "nymph",
   100,  000000000000000000000ULL,  1,   37,  3,  11, {{0,1}}},
    { "orc",
    15,  010000000000000000000ULL,  1,    5,  1,  14, {{1,8}}},
    { "phantom",
     0,  002000000000000000000ULL,  1,  120,  8,  17, {{4,4}}},
    { "quagga",
     0,  020000000000000000000ULL,  1,   15,  3,  17, {{1,5},{1,5}}},
    { "rattlesnake",
     0,  020000000000000000000ULL,  1,    9,  2,  17, {{1,6}}},
    { "snake",
     0,  000000000000000000000ULL,  1,    2,  1,  15, {{1,3}}},
    { "troll",
    50,  021000000000000000000ULL,  1,  120,  6,  16, {{1,8},{1,8},{2,6}}},
    { "black unicorn",
     0,  020000000000000000000ULL,  1,  190,  7,  22, {{1,9},{1,9},{2,9}}},
    { "vampire",
    20,  021000000000000000000ULL,  1,  350,  8,  19, {{1,10}}},
    { "wraith",
     0,  000000000000000000000ULL,  1,   55,  5,  16, {{1,6}}},
    { "xeroc",
    30,  000000000000000000000ULL,  1,  100,  7,  13, {{4,4}}},
    { "yeti",
    30,  000000000000000000000ULL,  1,   50,  4,  14, {{1,6},{1,6}}},
    { "zombie",
     0,  020000000000000000000ULL,  1,    6,  2,  12, {{1,8}}},
  };
}

void Monster::free_monsters() {
  delete monsters;
  monsters = nullptr;
}


int Monster::get_armor() const {
  return Character::get_armor();
}

string Monster::get_attack_string(bool successful_hit) const {

  size_t i = static_cast<size_t>(os_rand_range(4));

  if (successful_hit) {
    vector<string> hit_string {
        "hits"
      , "scored an excellent hit on"
      , "has injured"
      , "swings and hits"
    };
    return hit_string.at(i);

  } else {
    vector<string> miss_string {
        "misses"
      , "swings and misses"
      , "barely misses"
      , "doesn't hit"
    };
    return miss_string.at(i);
  }
}

string Monster::get_name() const {

  if (!monster_seen_by_player(this) && !player->can_sense_monsters()) {
    return "something";

  } else {
    stringstream ss;
    ss << "the " << monster_name_by_type(static_cast<char>(get_type()));
    return ss.str();
  }
}

void Monster::set_invisible() {

  Character::set_invisible();
  if (player->can_see(get_position()))
  {
    Game::io->message(get_name() + " disappeared");
    Game::io->print_tile(get_position());
  }
}

char Monster::random_monster_type() {

  /* List of monsters in rough order of vorpalness */
  string const mons =  "KEBSHIROZLCQANYFTWPXUMVGJD";
  int const default_spread = 5;

  // Formula for generating monster type
  int index = Game::current_level + (os_rand_range(10) - 6);

  // If we are in deep, only pick a default spread of the hardest monsters
  int mons_size = static_cast<int>(mons.size());
  if (index >= mons_size) {
    index = mons_size - default_spread + os_rand_range(default_spread);
  }

  // If we are in shallow, only pick a default spread of easy monsters
  if (index < 0) {
    index = os_rand_range(default_spread);
  }

  return mons.at(static_cast<size_t>(index));
}

// Experience to add for this monster's level/hit points
static int extra_experience(int level, int max_health) {

  int mod = level == 1
    ? max_health / 8
    : max_health / 6;

  if (level > 9)
    mod *= 20;
  else if (level > 6)
    mod *= 4;

  return mod;
}

Monster::~Monster() {}

Monster::Monster(char type, Coordinate const& pos, struct room* room) :
  Monster(type, pos, room, monsters->at(static_cast<size_t>(type - 'A')))
{}

Monster::Monster(char type, Coordinate const& pos, struct room* room,
                 monster_template const& m_template) :
  Character(10, m_template.m_basexp, m_template.m_level, m_template.m_armor,
            roll(m_template.m_level, 8), m_template.m_dmg, pos, room,
            m_template.m_flags, type),
  t_dest(), t_pack(), t_disguise(type), turns_not_moved(0),
  speed(m_template.m_speed) {

  // All monsters are equal, but some monsters are more equal than others, so
  // they also give more experience
  gain_experience(extra_experience(get_level(), get_max_health()));

  if (player != nullptr && player->has_ring_with_ability(Ring::Type::AGGR)) {
    monster_start_running(&pos);
  }

  if (type == 'X')
    t_disguise = rnd_thing();
}

void Monster::set_target(Coordinate const* new_target) {
  t_dest = new_target;
}

char Monster::get_disguise() const {
  return t_disguise;
}

Monster*
monster_notice_player(int y, int x)
{
  Monster *monster = Game::level->get_monster(x, y);

  /* Monster can begin chasing after the player if: */
  if (!monster->is_chasing()
      && monster->is_mean()
      && !monster->is_held()
      && !player->is_stealthy()
      && !os_rand_range(3))
  {
    monster->set_target(&player->get_position());
    if (!monster->is_stuck())
      monster->set_chasing();
  }

  /* Medusa can confuse player */
  if (monster->get_type() == 'M'
      && !player->is_blind()
      && !monster->is_found()
      && !monster->is_cancelled()
      && monster->is_chasing())
  {
    struct room const* rp = player->get_room();
    if ((rp != nullptr && !(rp->r_flags & ISDARK))
        || dist(y, x, player->get_position().y, player->get_position().x) < LAMPDIST)
    {
      monster->set_found();
      if (!player->saving_throw(VS_MAGIC))
      {
        Game::io->message(monster->get_name() + "'s gaze has confused you");
        player->set_confused();
      }
    }
  }

  /* Let greedy ones guard gold */
  if (monster->is_greedy() && !monster->is_chasing())
  {
    monster->set_target(player->get_room()->r_goldval
        ? &player->get_room()->r_gold
        : &player->get_position());
    monster->set_chasing();
  }

  return monster;
}

void
monster_give_pack(Monster* mon) {

  if (mon == nullptr) {
    error("Monster was null");
  }

  size_t monster_id = static_cast<size_t>(mon->get_type() - 'A');
  int carry_chance = Monster::monsters->at(monster_id).m_carry;

  if (Game::current_level >= Game::max_level_visited &&
      os_rand_range(100) < carry_chance) {
    mon->t_pack.push_back(Item::random());
  }
}

int
monster_save_throw(int which, Monster const* mon)
{
  assert(mon != nullptr);

  int need = 14 + which - mon->get_level() / 2;
  return (roll(1, 20) >= need);
}

void
monster_start_running(Coordinate const* runner)
{
  if (runner == nullptr) {
    error("runner was null");
  }

  Monster *tp = Game::level->get_monster(*runner);

  monster_find_new_target(tp);
  if (!tp->is_stuck())
  {
    tp->set_not_held();
    tp->set_chasing();
  }
}

void
monster_on_death(Monster** monster_ptr, bool print_message)
{
  if (monster_ptr == nullptr) {
    error("monster was null");
  } else if (*monster_ptr == nullptr) {
    error("*monster was null");
  }

  Monster* monster = *monster_ptr;

  player->gain_experience(monster->get_experience());

  switch (monster->get_type()) {
    /* If the monster was a venus flytrap, un-hold him */
    case 'F': {
      player->set_not_held();
      monster_flytrap_hit = 0;
    } break;

    /* Leprechauns drop gold */
    case 'L': {
      if (fallpos(&monster->get_position(), &monster->get_room()->r_gold)) {
        if (player->saving_throw(VS_MAGIC)) {
          int gold_amount =
            Gold::random_gold_amount() + Gold::random_gold_amount() +
            Gold::random_gold_amount() + Gold::random_gold_amount();
          monster->t_pack.push_back(new Gold(gold_amount));
        } else {
          monster->t_pack.push_back(new Gold());
        }
      }
    } break;
  }

  /* Get rid of the monster. */
  if (print_message) {
    Game::io->message("you have slain " + monster->get_name());
  }
  monster_remove_from_screen(monster_ptr, true);

  /* Do adjustments if he went up a level */
  player->check_for_level_up();
  if (fight_flush) {
    flushinp();
  }
}

void
monster_remove_from_screen(Monster** monster_ptr, bool was_killed)
{
  if (monster_ptr == nullptr) {
    error("monster was null");
  } else if (*monster_ptr == nullptr) {
    error("*monster was null");
  }

  Monster* monster = *monster_ptr;

  // If it was killed, drop stash. Otherwise just delete it
  for (Item* obj : monster->t_pack) {
    obj->set_position(monster->get_position());
    if (was_killed) {
      weapon_missile_fall(obj, false);
    } else {
      delete obj;
    }
  }
  monster->t_pack.clear();

  Coordinate position = monster->get_position();
  Game::level->set_monster(position, nullptr);
  Game::level->monsters.remove(monster);

  Game::io->print_tile(position.x, position.y);
  if (monster->is_players_target()) {
    to_death = false;
    if (fight_flush)
      flushinp();
  }

  delete monster;
  *monster_ptr = nullptr;
}

void
monster_teleport(Monster* monster, Coordinate const* destination)
{
  Coordinate old_position = monster->get_position();

  /* Select destination */
  Coordinate new_pos;
  if (destination == nullptr)
    do
      Game::level->get_random_room_coord(nullptr, &new_pos, 0, true);
    while (new_pos == monster->get_position());
  else
    new_pos = *destination;

  Game::level->set_monster(monster->get_position(), nullptr);

  /* Add monster */
  monster->set_room(Game::level->get_room(new_pos));
  monster->set_position(new_pos);
  monster->set_not_held();

  if (monster_seen_by_player(monster))
    Game::io->print_color(new_pos.x, new_pos.y, monster->t_disguise);
  else if (player->can_sense_monsters()) {
    standout();
    Game::io->print_color(new_pos.x, new_pos.y, monster->get_type());
    standend();
  }

  /* Remove monster */
  if (monster_seen_by_player(monster)) {
    Game::io->print_tile(old_position);
  }
}

void
monster_do_special_ability(Monster** monster)
{
  assert(monster != nullptr);
  assert(*monster != nullptr);

  if ((*monster)->is_cancelled())
    return;

  switch ((*monster)->get_type())
  {
    /* If an aquator hits, you can lose armor class */
    case 'A': {
      player->rust_armor();
    } return;

    /* Venus Flytrap stops the poor guy from moving */
    case 'F': {
      player->set_held();
      ++monster_flytrap_hit;
      player->take_damage(1);
      if (player->get_health() <= 0) {
        death('F');
      }
    } return;

    /* The ice monster freezes you */
    case 'I': {
      player->set_not_running();
      if (!player_turns_without_action) {
        Game::io->message("you are frozen by the " + (*monster)->get_name());
      }
      player_turns_without_action += os_rand_range(2) + 2;
      if (player_turns_without_action > 50) {
        death(DEATH_ICE);
      }
    } return;


    /* Leperachaun steals some gold and disappears */
    case 'L': {
      monster_remove_from_screen(monster, false);
      *monster = nullptr;

      player->give_gold(-Gold::random_gold_amount());
      if (!player->saving_throw(VS_MAGIC)) {
        player->give_gold(-Gold::random_gold_amount() + Gold::random_gold_amount() +
                      Gold::random_gold_amount() + Gold::random_gold_amount());
      }

      if (player->get_gold() < 0) {
        player->give_gold(-player->get_gold());
      }
      Game::io->message("your pack_gold feels lighter");
    } return;


    /* Nymph's steal a magic item and disappears */
    case 'N': {
      Item* steal = player->pack_find_magic_item();
      if (steal != nullptr) {
        monster_remove_from_screen(monster, false);
        *monster = nullptr;
        player->pack_remove(steal, false, false);
        Game::io->message("your pack feels lighter");
        delete steal;
      }
    } return;

    /* Rattlesnakes have poisonous bites */
    case 'R': {
      if (!player->saving_throw(VS_POISON)
          && !player->has_ring_with_ability(Ring::Type::SUSTSTR)) {
        player->modify_strength(-1);
        Game::io->message("you feel weaker");
      }
    } return;

    /* Vampires can steal max hp */
    case 'V': {
      if (os_rand_range(100) < 30) {
        int fewer = roll(1, 3);
        player->take_damage(fewer);
        player->modify_max_health(-fewer);
        if (player->get_health() <= 0) {
          death('V');
        }
        Game::io->message("you feel weaker");
      }
    } return;

    /* Wraiths might drain exp */
    case 'W': {
      if (os_rand_range(100) < 15) {
        if (player->get_level() == 1) {
          death('W');  /* Death by no level */
        }
        player->lower_level(1);

        int fewer = roll(1, 10);
        player->take_damage(fewer);
        player->modify_max_health(-fewer);
        if (player->get_health() <= 0) {
          death('W');
        }
        Game::io->message("you feel weaker");
      }
    } return;

    default: return;
  }
}

string const&
monster_name_by_type(char monster_type)
{
  return Monster::monsters->at(static_cast<size_t>(monster_type - 'A')).m_name;
}

bool
monster_seen_by_player(Monster const* monster)
{
  assert(monster != nullptr);

  Coordinate const& monster_pos = monster->get_position();
  int monster_y = monster_pos.y;
  int monster_x = monster_pos.x;

  // Special cases when not seen
  if (player->is_blind() ||
      (monster->is_invisible() && !player->has_true_sight()))
    return false;

  // Dark place ?
  int dist = dist_cp(&monster_pos, &player->get_position());
  if (dist < LAMPDIST)
  //if (dist_cp(&monster_pos, &player->get_position()) < LAMPDIST)
  {
    if (monster_y != player->get_position().y && monster_x != player->get_position().x
        && !Game::level->can_step(player->get_position().x, monster_y)
        && !Game::level->can_step(monster_x, player->get_position().y))
      return false;
    return true;
  }

  if (monster->get_room() != player->get_room())
    return false;
  return !(monster->get_room()->r_flags & ISDARK);
}

bool
monster_is_anyone_seen_by_player(void)
{
  for (Monster* mon : Game::level->monsters) {
    if (monster_seen_by_player(mon)) {
      return true;
    }
  }
  return false;
}

void
monster_show_all_as_trippy(void)
{
  bool seemonst = player->can_sense_monsters();
  for (Monster const* tp : Game::level->monsters) {

    if (monster_seen_by_player(tp)) {
      chtype symbol = (tp->get_type() == 'X' && tp->t_disguise != 'X')
        ? static_cast<chtype>(rnd_thing())
        : static_cast<chtype>(os_rand_range(26) + 'A');
      Game::io->print_color(tp->get_position().x, tp->get_position().y, symbol);
    }
    else if (seemonst) {
      standout();
      Game::io->print_color(tp->get_position().x, tp->get_position().y,
          os_rand_range(26) + 'A');
      standend();
    }
  }
}

void
monster_move_all(void)
{
  // This function needs a manual loop, since monsters can die
  auto it = Game::level->monsters.begin();
  while (it != Game::level->monsters.end()) {
    Monster* mon = *it++;

    // Speed < 0 means one move each x turns
    int speed = mon->get_speed();
    if (speed < 0) {
      if (mon->turns_not_moved >= -speed) {
        mon->turns_not_moved = 0;
        speed = 1;
      } else {
        ++mon->turns_not_moved;
      }
    }

    // One move per turn
    for (int i = 0; i < speed; ++i) {
      bool wastarget = mon->is_players_target();
      Coordinate orig_pos = mon->get_position();

      if (!monster_take_turn(mon)) {
        // Monster is dead
        break;
      }

      if (wastarget && !(orig_pos == mon->get_position()))
      {
        mon->set_not_players_target();
        to_death = false;
      }
    }
  }
}

void
monster_aggravate_all(void)
{
  for (Monster* mon : Game::level->monsters) {
    monster_start_running(&mon->get_position());
  }
}

void
monster_show_all_hidden(void)
{
  for (Monster* mon : Game::level->monsters) {
    if (mon->is_invisible() && monster_seen_by_player(mon)) {
      Game::io->print_color(mon->get_position().x, mon->get_position().y,
          mon->t_disguise);
    }
  }
}

void
monster_aggro_all_which_desire_item(Item* item)
{
  for (Monster* mon : Game::level->monsters) {
    if (mon->t_dest == &item->get_position()) {
      mon->t_dest = &player->get_position();
    }
  }
}

void
monster_hide_all_invisible(void)
{
  for (Monster* mon : Game::level->monsters) {
    if (mon->is_invisible() && monster_seen_by_player(mon)) {
      Game::io->print_tile(mon->get_position());
    }
  }
}

bool
monster_sense_all_hidden(void)
{
  bool spotted_something = false;
  for (Monster* mon : Game::level->monsters) {
    if (!monster_seen_by_player(mon)) {
      standout();
      Game::io->print_color(mon->get_position().x, mon->get_position().y,
           mon->get_type());
      standend();
      spotted_something = true;
    }
  }
  return spotted_something;
}

void
monster_unsense_all_hidden(void)
{
  for (Monster* mon : Game::level->monsters) {
    if (!monster_seen_by_player(mon)) {
      Game::io->print_tile(mon->get_position());
    }
  }
}

bool
monster_show_if_magic_inventory(void)
{
  bool atleast_one = false;
  for (Monster* mon : Game::level->monsters) {
    for (Item* item : mon->t_pack) {
      if (item->is_magic())
      {
        Coordinate pos = mon->get_position();
        atleast_one = true;
        mvwaddcch(Game::io->extra_screen, pos.y, pos.x, IO::Magic);
      }
    }
  }
  return atleast_one;
}

void
monster_polymorph(Monster* target)
{
  assert(target != nullptr);
  stringstream os;

  // Venus Flytrap loses its grip
  if (target->get_type() == 'F')
    player->set_not_held();


  Coordinate pos = target->get_position();
  bool was_seen = monster_seen_by_player(target);
  if (was_seen)
  {
    Game::io->print_color(pos.x, pos.y, Game::level->get_tile(pos));
    os << target->get_name();
  }

  // Save some things from old monster
  list<Item*> target_pack = target->t_pack;

  // Generate the new monster
  char monster = static_cast<char>(os_rand_range(26) + 'A');
  bool same_monster = monster == target->get_type();

  // TODO: Test this. Important that pack and damage gets copied
  *target = Monster(monster, pos, Game::level->get_room(pos));

  if (monster_seen_by_player(target))
  {
    Game::io->print_color(pos.x, pos.y, monster);
    if (same_monster)
    {
      os << " now looks a bit different";
      Game::io->message(os.str());
    }
    else
    {
      os << " turned into a " << target->get_name();
      Game::io->message(os.str());
    }
  }
  else if (was_seen)
  {
    os << " disappeared";
    Game::io->message(os.str());
  }

  // Put back some saved things from old monster
  target->t_pack = target_pack;
}

bool monster_try_breathe_fire_on_player(Monster const& monster) {

  // For dragons, check and see if
  // (a) the hero is on a straight line from it, and
  // (b) that it is within shooting distance, but outside of striking range

  Coordinate mon_coord = monster.get_position();
  Coordinate player_coord = player->get_position();
  int const dragonshot_chance = 5;
  if (monster.get_type() == 'D' &&
      (mon_coord.y == player_coord.y ||
       mon_coord.x == player_coord.x ||
       abs(mon_coord.y - player_coord.y) == abs(mon_coord.x - player_coord.x)) &&
      dist_cp(&mon_coord, &player_coord) <= BOLT_LENGTH * BOLT_LENGTH &&
      !monster.is_cancelled() && os_rand_range(dragonshot_chance) == 0) {

    Coordinate delta(sign(player_coord.x - mon_coord.x),
        sign(player_coord.y - mon_coord.y));
    Coordinate position = monster.get_position();
    magic_bolt(&position, &delta, "flame");
    command_stop(true);
    Daemons::daemon_reset_doctor();
    if (to_death && !monster.is_players_target()) {
      to_death = false;
    }
    return true;
  }

  return false;
}

int Monster::get_speed() const {
  return speed;
}

void Monster::increase_speed() {
  if (++speed == 0) {
    speed = 1;
  }
  turns_not_moved = 0;
}

void Monster::decrease_speed() {
  if (--speed) {
    speed = -1;
  }
  turns_not_moved = 0;
}
