#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <string>
#include <vector>
#include <sstream>

using namespace std;

#include "error_handling.h"
#include "game.h"
#include "io.h"
#include "pack.h"
#include "scrolls.h"
#include "rings.h"
#include "level_rooms.h"
#include "misc.h"
#include "level.h"
#include "player.h"
#include "weapons.h"
#include "things.h"
#include "os.h"
#include "armor.h"
#include "options.h"
#include "rogue.h"
#include "death.h"

#include "monster.h"
#include "monster_private.h"

int            monster_flytrap_hit = 0; // Number of time flytrap has hit

#define ISGREED 0000040
#define ISINVIS 0002000
#define ISMEAN  0004000
#define ISFLY   0040000
#define ISREGEN 0010000

vector<monster_template> const monsters {
  /* Name        CARRY  FLAG                   exp lvl  amr  dmg              */
  { "aquator",       0, ISMEAN,                 20,  5,  18, {{0,1}}},
  { "bat",           0, ISFLY,                   1,  1,  17, {{1,2}}},
  { "centaur",      15, 0,                      17,  4,  16, {{1,2},{1,5},{1,5}}},
  { "dragon",      100, ISMEAN,               5000, 10,  21, {{1,8},{1,8},{3,10}}},
  { "emu",           0, ISMEAN,                  2,  1,  13, {{1,2}}},
  { "venus flytrap", 0, ISMEAN,                 80,  8,  17, {{0,1}}},
  { "griffin",      20, ISMEAN|ISFLY|ISREGEN, 2000, 13,  18, {{4,3},{3,5}}},
  { "hobgoblin",     0, ISMEAN,                  3,  1,  15, {{1,8}}},
  { "ice monster",   0, 0,                       5,  1,  11, {{0,1}}},
  { "jabberwock",   70, 0,                    3000, 15,  14, {{2,12},{2,4}}},
  { "kestrel",       0, ISMEAN|ISFLY,            1,  1,  13, {{1,4}}},
  { "leprechaun",    0, 0,                      10,  3,  12, {{1,1}}},
  { "medusa",       40, ISMEAN,                200,  8,  18, {{3,4},{3,4},{2,5}}},
  { "nymph",       100, 0,                      37,  3,  11, {{0,1}}},
  { "orc",          15, ISGREED,                 5,  1,  14, {{1,8}}},
  { "phantom",       0, ISINVIS,               120,  8,  17, {{4,4}}},
  { "quagga",        0, ISMEAN,                 15,  3,  17, {{1,5},{1,5}}},
  { "rattlesnake",   0, ISMEAN,                  9,  2,  17, {{1,6}}},
  { "snake",         0, ISMEAN,                  2,  1,  15, {{1,3}}},
  { "troll",        50, ISREGEN|ISMEAN,        120,  6,  16, {{1,8},{1,8},{2,6}}},
  { "black unicorn", 0, ISMEAN,                190,  7,  22, {{1,9},{1,9},{2,9}}},
  { "vampire",      20, ISREGEN|ISMEAN,        350,  8,  19, {{1,10}}},
  { "wraith",        0, 0,                      55,  5,  16, {{1,6}}},
  { "xeroc",        30, 0,                     100,  7,  13, {{4,4}}},
  { "yeti",         30, 0,                      50,  4,  14, {{1,6},{1,6}}},
  { "zombie",        0, ISMEAN,                  6,  2,  12, {{1,8}}},
};



int Monster::get_armor() const {
  return Character::get_armor();
}

void Monster::set_oldch(Coordinate &coord) {
  char old_char = t_oldch;

  if (get_position() == coord) {
    return;
  }

  t_oldch = static_cast<char>(mvincch(coord.y, coord.x));
  if (!player->is_blind()) {

    if ((old_char == FLOOR || t_oldch == FLOOR) &&
        (get_room()->r_flags & ISDARK)) {
      t_oldch = SHADOW;

    } else if (dist_cp(&coord, &player->get_position()) <= LAMPDIST) {
      t_oldch = Game::level->get_ch(coord);
    }
  }
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

  } else if (player->is_hallucinating()) {

    int ch = mvincch(get_position().y, get_position().x);
    if (!isupper(ch)) {
      ch = static_cast<int>(os_rand_range(monsters.size()));
    } else {
      ch -= 'A';
    }

    stringstream ss;
    ss << "the " << monster_name_by_type(static_cast<char>(ch));
    return ss.str();

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
    io_msg("%s disappeared", get_name().c_str());
    mvaddcch(get_position().y, get_position().x, static_cast<chtype>(t_oldch));
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

Monster::Monster(char type, Coordinate const& pos, struct room* room) :
  Monster(type, pos, room, monsters.at(static_cast<size_t>(type - 'A')))
{}

Monster::Monster(char type, Coordinate const& pos, struct room* room,
                 monster_template const& m_template) :
  Character(10, m_template.m_basexp, m_template.m_level, m_template.m_armor,
            roll(m_template.m_level, 8), m_template.m_dmg, pos, room,
            m_template.m_flags, type),
  t_dest(), t_pack(), t_disguise(type),
  t_oldch(static_cast<char>(mvincch(pos.y, pos.x))), t_turn(true) {

  // All monsters are equal, but some monsters are more equal than others, so
  // they also give more experience
  gain_experience(extra_experience(get_level(), get_max_health()));

  if (player->has_ring_with_ability(R_AGGR)) {
    monster_start_running(&pos);
  }

  if (type == 'X')
    t_disguise = rnd_thing();
}

void Monster::set_target(Coordinate const* new_target) {
  t_dest = new_target;
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
      && !player->is_hallucinating()
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
        io_msg("%s's gaze has confused you", monster->get_name().c_str());
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
  int carry_chance = monsters.at(monster_id).m_carry;

  if (Game::current_level >= Game::max_level_visited &&
      os_rand_range(100) < carry_chance) {
    mon->t_pack.push_back(new_thing());
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
monster_on_death(Monster* monster, bool pr)
{
  assert(monster != nullptr);

  player->gain_experience(monster->get_experience());

  switch (monster->get_type())
  {
    /* If the monster was a venus flytrap, un-hold him */
    case 'F':
      player->set_not_held();
      monster_flytrap_hit = 0;
      break;

    /* Leprechauns drop gold */
    case 'L':
      if (fallpos(&monster->get_position(), &monster->get_room()->r_gold))
      {
        Item* gold = new Item();
        gold->o_type = GOLD;
        gold->o_goldval = GOLDCALC;
        if (player->saving_throw(VS_MAGIC))
          gold->o_goldval += GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
        monster->t_pack.push_back(gold);
      }
  }

  /* Get rid of the monster. */
  if (pr)
    io_msg("you have slain %s", monster->get_name().c_str());
  monster_remove_from_screen(&monster->get_position(), monster, true);

  /* Do adjustments if he went up a level */
  player->check_for_level_up();
  if (fight_flush)
    flushinp();
}

void
monster_remove_from_screen(Coordinate const* mp, Monster* tp, bool waskill)
{
  assert(mp != nullptr);
  assert(tp != nullptr);

  for (Item* obj : tp->t_pack) {
    obj->set_pos(tp->get_position());
    if (waskill)
      weapon_missile_fall(obj, false);
    else
      delete obj;
  }
  tp->t_pack.clear();

  Game::level->set_monster(*mp, nullptr);
  mvaddcch(mp->y, mp->x, static_cast<chtype>(tp->t_oldch));

  Game::level->monsters.remove(tp);

  if (tp->is_players_target())
  {
    to_death = false;
    if (fight_flush)
      flushinp();
  }

  delete tp;
}

bool
monster_is_dead(Monster const* monster)
{
  if (monster == nullptr)
    return true;

  return !(find(Game::level->monsters.cbegin(), Game::level->monsters.cend(), monster) ==
          Game::level->monsters.cend());
}

void
monster_teleport(Monster* monster, Coordinate const* destination)
{
  /* Select destination */
  Coordinate new_pos;
  if (destination == nullptr)
    do
      Game::level->get_random_room_coord(nullptr, &new_pos, 0, true);
    while (new_pos == monster->get_position());
  else
    new_pos = *destination;

  /* Remove monster */
  if (monster_seen_by_player(monster))
    mvaddcch(monster->get_position().y, monster->get_position().x, static_cast<chtype>(monster->t_oldch));
  monster->set_oldch(new_pos);
  Game::level->set_monster(monster->get_position(), nullptr);

  /* Add monster */
  monster->set_room(Game::level->get_room(new_pos));
  monster->set_position(new_pos);
  monster->set_not_held();

  if (monster_seen_by_player(monster))
    mvaddcch(new_pos.y, new_pos.x, static_cast<chtype>(monster->t_disguise));
  else if (player->can_sense_monsters())
    mvaddcch(new_pos.y, new_pos.x, static_cast<chtype>(monster->get_type())| A_STANDOUT);
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
    case 'A':
      armor_rust();
      return;

    /* Venus Flytrap stops the poor guy from moving */
    case 'F':
      player->set_held();
      ++monster_flytrap_hit;
      player->take_damage(1);
      if (player->get_health() <= 0)
        death('F');
      return;

    /* The ice monster freezes you */
    case 'I':
      player->set_not_running();
      if (!player_turns_without_action)
      {
        io_msg("you are frozen by the %s", (*monster)->get_name().c_str());
      }
      player_turns_without_action += os_rand_range(2) + 2;
      if (player_turns_without_action > 50)
        death(DEATH_ICE);
      return;


    /* Leperachaun steals some gold and disappears */
    case 'L':
      monster_remove_from_screen(&(*monster)->get_position(), *monster, false);
      *monster = nullptr;

      pack_gold -= GOLDCALC;
      if (!player->saving_throw(VS_MAGIC))
        pack_gold -= GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
      if (pack_gold < 0)
        pack_gold = 0;
      io_msg("your pack_gold feels lighter");
      return;


    /* Nymph's steal a magic item and disappears */
    case 'N': {
      Item* steal = pack_find_magic_item();
      if (steal != nullptr)
      {
        monster_remove_from_screen(&(*monster)->get_position(), *monster, false);
        *monster = nullptr;
        pack_remove(steal, false, false);
        io_msg("your pack feels lighter");
        delete steal;
      }
      return;
      }

    /* Rattlesnakes have poisonous bites */
    case 'R':
      if (!player->saving_throw(VS_POISON)
          && !player->has_ring_with_ability(R_SUSTSTR))
      {
        player->modify_strength(-1);
        io_msg("you feel weaker");
      }
      return;

    /* Vampires can steal max hp */
    case 'V':
      if (os_rand_range(100) < 30)
      {
        int fewer = roll(1, 3);
        player->take_damage(fewer);
        player->modify_max_health(-fewer);
        if (player->get_health() <= 0)
          death('V');
        io_msg("you feel weaker");
      }
      return;

    /* Wraiths might drain exp */
    case 'W':
      if (os_rand_range(100) < 15)
      {
        if (player->get_level() == 1)
          death('W');  /* Death by no level */
        player->lower_level(1);

        int fewer = roll(1, 10);
        player->take_damage(fewer);
        player->modify_max_health(-fewer);
        if (player->get_health() <= 0)
          death('W');
        io_msg("you feel weaker");
      }
      return;

    default: return;
  }
}

string const&
monster_name_by_type(char monster_type)
{
  return monsters.at(static_cast<size_t>(monster_type - 'A')).m_name;
}

bool
monster_seen_by_player(Monster const* monster)
{
  assert(monster != nullptr);

  int monster_y = monster->get_position().y;
  int monster_x = monster->get_position().x;

  if (player->is_blind() ||
      (monster->is_invisible() && !player->has_true_sight()))
    return false;

  if (dist(monster_y, monster_x, player->get_position().y, player->get_position().x) < LAMPDIST)
  {
    if (monster_y != player->get_position().y && monster_x != player->get_position().x
        && !step_ok(Game::level->get_ch(player->get_position().x, monster_y))
        && !step_ok(Game::level->get_ch(monster_x, player->get_position().y)))
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
      mvaddcch(tp->get_position().y, tp->get_position().x, symbol);
    }
    else if (seemonst) {
      mvaddcch(tp->get_position().y, tp->get_position().x,
          static_cast<chtype>(os_rand_range(26) + 'A') | A_STANDOUT);
    }
  }
}

void
monster_move_all(void)
{
  for (Monster* mon : Game::level->monsters) {

    if (!mon->is_held() && mon->is_chasing())
    {
      bool wastarget = mon->is_players_target();
      Coordinate orig_pos = mon->get_position();
      if (!monster_chase(mon))
        continue;

      if (mon->is_flying()
          && dist_cp(&player->get_position(), &mon->get_position()) >= 3)
        monster_chase(mon);

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
    if (mon->is_invisible() && monster_seen_by_player(mon)
        && !player->is_hallucinating())
      mvaddcch(mon->get_position().y, mon->get_position().x, static_cast<chtype>(mon->t_disguise));
  }
}

void
monster_aggro_all_which_desire_item(Item* item)
{
  for (Monster* mon : Game::level->monsters) {
    if (mon->t_dest == &item->get_pos()) {
      mon->t_dest = &player->get_position();
    }
  }
}

void
monster_hide_all_invisible(void)
{
  for (Monster* mon : Game::level->monsters) {
    if (mon->is_invisible() && monster_seen_by_player(mon)) {
      mvaddcch(mon->get_position().y, mon->get_position().x, static_cast<chtype>(mon->t_oldch));
    }
  }
}

bool
monster_sense_all_hidden(void)
{
  bool spotted_something = false;
  for (Monster* mon : Game::level->monsters) {
    if (!monster_seen_by_player(mon)) {
      mvaddcch(mon->get_position().y, mon->get_position().x,
          (player->is_hallucinating()
           ? static_cast<chtype>(os_rand_range(26) + 'A')
           : static_cast<chtype>(mon->get_type()))
            | A_STANDOUT);
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
      mvaddcch(mon->get_position().y, mon->get_position().x, static_cast<chtype>(mon->t_oldch));
    }
  }
}

void
monster_print_all(void)
{
  for (Monster* mon : Game::level->monsters) {

    if (player->can_see(mon->get_position())) {
      chtype symbol = (!mon->is_invisible() || player->has_true_sight())
        ? static_cast<chtype>(mon->t_disguise)
        : static_cast<chtype>(Game::level->get_ch(mon->get_position()));
      mvaddcch(mon->get_position().y, mon->get_position().x, symbol);

    } else if (player->can_sense_monsters()) {
      mvaddcch(mon->get_position().y, mon->get_position().x,
               static_cast<chtype>(mon->get_type())| A_STANDOUT);
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
        atleast_one = true;
        mvwaddcch(hw, mon->get_position().y, mon->get_position().x, MAGIC);
      }
    }
  }
  return atleast_one;
}

int
monster_add_nearby(Monster** nearby_monsters, struct room const* room)
{
  assert(nearby_monsters != nullptr);
  bool inpass = player->get_room()->r_flags & ISGONE;
  Monster** nearby_monsters_start = nearby_monsters;

  for (Monster* mon : Game::level->monsters) {
    if (mon->get_room() == player->get_room()
        || mon->get_room() == room
        ||(inpass && Game::level->get_ch(mon->get_position()) == DOOR &&
          Game::level->get_passage(mon->get_position()) == player->get_room())) {
      *nearby_monsters++ = mon;
    }
  }

  return static_cast<int>(nearby_monsters_start - nearby_monsters);
}

void
monster_polymorph(Monster* target)
{
  assert(target != nullptr);

  // Venus Flytrap loses its grip
  if (target->get_type() == 'F')
    player->set_not_held();


  Coordinate pos = target->get_position();
  bool was_seen = monster_seen_by_player(target);
  if (was_seen)
  {
    mvaddcch(pos.y, pos.x, static_cast<chtype>(Game::level->get_ch(pos)));
    io_msg_add("%s", target->get_name().c_str());
  }

  // Save some things from old monster
  char oldch = target->t_oldch;
  list<Item*> target_pack = target->t_pack;

  // Generate the new monster
  char monster = static_cast<char>(os_rand_range(26) + 'A');
  bool same_monster = monster == target->get_type();

  // TODO: Test this. Important that pack and damage gets copied
  *target = Monster(monster, pos, Game::level->get_room(pos));

  if (monster_seen_by_player(target))
  {
    mvaddcch(pos.y, pos.x, static_cast<chtype>(monster));
    if (same_monster)
      io_msg(" now looks a bit different");
    else
    {
      io_msg(" turned into a %s", target->get_name().c_str());
    }
  }
  else if (was_seen)
    io_msg(" disappeared");

  // Put back some saved things from old monster
  target->t_oldch = oldch;
  target->t_pack = target_pack;
}
