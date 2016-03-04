#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

#include "gold.h"
#include "disk.h"
#include "magic.h"
#include "command.h"
#include "daemons.h"
#include "error_handling.h"
#include "game.h"
#include "io.h"
#include "scrolls.h"
#include "rings.h"
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

  if (!player->can_see(*this) && !player->can_sense_monsters()) {
    return "something";

  } else {
    stringstream ss;
    ss << "the " << name(subtype);
    return ss.str();
  }
}

void Monster::set_invisible() {
  if (player->can_see(get_position())) {
    Game::io->message(get_name() + " disappeared");
  }

  Character::set_invisible();
}

Monster::Type Monster::random_monster_type() {
  return static_cast<Monster::Type>(os_rand_range(Monster::NMONSTERS));
}


Monster::Type Monster::random_monster_type_for_level() {
  vector<Type> mon_types;

  switch (Game::current_level) {
    default:
      mon_types.push_back(AdultRedDragon);
      mon_types.push_back(Jabberwock);

    [[clang::fallthrough]];
    case 24: case 25: {
    } break;


    case 21: case 22: case 23: {
    } break;

    // Level 14-20: Gates of Moonlight
    case 18: case 19: case 20:
      mon_types.push_back(Erinyes);
      mon_types.push_back(GenieEfreeti);
      mon_types.push_back(GenieMarid);

    [[clang::fallthrough]];
    case 16: case 17:
      mon_types.push_back(ShadowDemon);
      mon_types.push_back(GenieVizier);
      mon_types.push_back(GenieShaitan);

    [[clang::fallthrough]];
    case 14: case 15:
      mon_types.push_back(GenieDjinni);
      mon_types.push_back(GenieJanni);
      mon_types.push_back(Nightmare);
      break;

    // Level 8-13: The Pale Expanse
    case 11: case 12: case 13:
      mon_types.push_back(Troll);
      mon_types.push_back(IceMonster);
      mon_types.push_back(Yeti);

    [[clang::fallthrough]];
    case 8: case 9: case 10:
      mon_types.push_back(Aquator);
      mon_types.push_back(Worg);
      mon_types.push_back(Ogre);
      mon_types.push_back(SkeletonHuman);
      mon_types.push_back(Quasit);
      break;

    // Level 1-7: The Troubled Caves
    case 6: case 7:
      mon_types.push_back(Quasit);
      mon_types.push_back(Worg);

    [[clang::fallthrough]];
    case 3: case 4: case 5:
      mon_types.push_back(Hobgoblin);
      mon_types.push_back(ZombieHuman);

    [[clang::fallthrough]];
    case 1: case 2:
      mon_types.push_back(SkeletonHuman);
      mon_types.push_back(Kobold);
      mon_types.push_back(Goblin);
      mon_types.push_back(Orc);
      mon_types.push_back(Bat);
      break;
  }

  // If we found something, pick one at random
  if (!mon_types.empty()) {
    return mon_types.at(os_rand_range(mon_types.size()));

  // Hmm, otherwise just lets pick the last one
  } else {
    return static_cast<Type>(NMONSTERS - 1);
  }
}

Monster::~Monster() {
  for (Item* item : pack) {
    delete item;
  }
}

Monster::Monster(int str, int dex, int con, int int_, int wis, int cha, int exp,
    int lvl, int ac,  int hp, std::vector<damage> const& dmg,
    Coordinate const& pos, std::vector<Feat> const& feats, int speed,
    Race race, char look_, char disguise_, Type subtype_, Coordinate const* target_,
    std::list<Item*> pack_)
  : Character(str, dex, con, int_, wis, cha, exp, lvl, ac, hp, dmg, pos, feats,
              speed, race),
  look{look_}, disguise{disguise_}, subtype{subtype_}, target{target_},
  pack{pack_}
{}

Monster::Monster(Monster::Type subtype_, Coordinate const& pos) {
  switch (subtype_) {
    case Bat: {
      *this = Monster(3, 15, 11, 2, 14, 4, 100, 1, 14, 4, vector<damage>{{1,2}},
          pos, vector<Feat>{}, 2, Race::Animal,
          'b', 'b', Monster::Bat, nullptr, list<Item*>{});
    } break;

    case Goblin: {
      *this =  Monster(11, 15, 12, 10, 9, 6, 135, 1, 14, 10, vector<damage>{{1,4}},
          pos, vector<Feat>{AttacksOnSight}, 1, Race::Goblinoid,
          'g', 'g', Monster::Goblin, nullptr, Item::random_items_chance(10, 1, 1));
    } break;

    case Kobold: {
      *this = Monster(17, 11, 12, 7, 8, 6, 135, 1, 13, 10, vector<damage>{{2,4}},
      pos, vector<Feat>{AttacksOnSight}, 1, Race::Orc,
      'o', 'o', Monster::Orc, nullptr, Item::random_items_chance(10, 1, 1));
    } break;

    case Orc: {
      *this = Monster(9, 13, 10, 10, 9, 8, 100, 1, 14, 8, vector<damage>{{1,6}},
      pos, vector<Feat>{AttacksOnSight}, 1, Race::Reptilian,
      'k', 'k', Monster::Kobold, nullptr, Item::random_items_chance(10, 1, 1));
    } break;

    case SkeletonHuman: {
      *this = Monster(15, 14, 10, 10, 10, 10, 135, 1, 14, 8, vector<damage>{{1,6}},
      pos, vector<Feat>{AttacksOnSight}, 1, Race::Undead,
      's', 's', Monster::SkeletonHuman, nullptr, Item::random_items_chance(10, 1, 1));
    } break;

    case ZombieHuman: {
      *this = Monster(17, 10, 10, 10, 10, 10, 200, 2, 12, 8, vector<damage>{{1,6}},
      pos, vector<Feat>{AttacksOnSight}, 1, Race::Undead,
      'z', 'z', Monster::ZombieHuman, nullptr, {});
    } break;

    case Hobgoblin: {
      *this = Monster(15, 15, 16, 10, 12, 8, 200, 1, 14, 10, vector<damage>{{1,8}},
      pos, vector<Feat>{AttacksOnSight}, 1, Race::Goblinoid,
      'h', 'h', Monster::Hobgoblin, nullptr, Item::random_items_chance(10, 1, 1));
    } break;

    case Quasit: {
      *this = Monster(8, 14, 11, 11, 12, 11, 600, 3, 14, 10, vector<damage>{{1,3}},
      pos, vector<Feat>{}, 1, Race::Outsider,
      'q', 'q', Monster::Quasit, nullptr, {});
    } break;

    case Worg: {
      *this = Monster(17, 15, 13, 6, 14, 10, 600, 4, 12, 10, vector<damage>{{1,6}},
      pos, vector<Feat>{AttacksOnSight}, 1, Race::MagicBeast,
      'w', 'w', Monster::Worg, nullptr, {});
    } break;

    case Ogre: {
      *this = Monster(21, 8, 15, 6, 10, 7, 800, 4, 18, 8, vector<damage>{{2,8}},
      pos, vector<Feat>{AttacksOnSight}, 1, Race::Giant,
      'O', 'O', Monster::Ogre, nullptr, {});
    } break;

    case IceMonster: {
      *this = Monster(16, 8, 15, 2, 13, 11, 1600, 7, 18, 10, vector<damage>{{1,8}},
      pos, vector<Feat>{AttackFreezesTarget}, 1, Race::MagicBeast,
      'I', 'I', Monster::IceMonster, nullptr, list<Item*>{});
    } break;

    case Yeti: {
      *this = Monster(19, 12, 15, 9, 12, 10, 1200, 6, 16, 10, vector<damage>{{2,6}},
      pos, vector<Feat>{AttacksOnSight}, 1, Race::MonstrousHumanoid,
      'y', 'y', Monster::Yeti, nullptr, list<Item*>{});
    } break;

    case Aquator: {
      *this = Monster(10, 17, 13, 2, 13, 8, 800, 5, 15, 8, vector<damage>{{1,3}},
      pos, vector<Feat>{AttacksOnSight, AttackRuinsMetal}, 1, Race::Aberration,
      'a', 'a', Monster::Aquator, nullptr, list<Item*>{});
    } break;

    case Troll: {
      *this = Monster(21, 14, 23, 6, 9, 6, 1600, 6, 14, 8, vector<damage>{{1,8},{1,6}},
      pos, vector<Feat>{Regenerating5}, 1, Race::Giant,
      'T', 'T', Monster::Troll, nullptr, list<Item*>{});
    } break;

    case Erinyes: {
      *this = Monster(20, 23, 21, 14, 18, 21, 4800, 10, 17, 10,
      vector<damage>{{1,8}},
      pos, vector<Feat>{AttacksOnSight}, 1, Race::Outsider,
      'e', 'e', Monster::Erinyes, nullptr, Item::random_items_chance(20, 1, 1));
    } break;

    case ShadowDemon: {
      *this = Monster(10, 18, 17, 14, 14, 19, 3200, 7, 14, 10,
      vector<damage>{{1,6},{1,6}},
      pos, vector<Feat>{AttacksOnSight, PermanentlyInvisible}, 1, Race::Outsider,
      'S', 'S', Monster::ShadowDemon, nullptr, {});
    } break;

    case Nightmare: {
      *this = Monster(18, 15, 16, 13, 13, 12, 1600, 6, 17, 10,
      vector<damage>{{1,6},{1,6}},
      pos, vector<Feat>{AttacksOnSight}, 1, Race::Outsider,
      'n', 'n', Monster::Nightmare, nullptr, list<Item*>{});
    } break;

    case GenieDjinni: {
      *this = Monster(18, 19, 14, 14, 15, 15, 1600, 7, 15, 10,
      vector<damage>{{1,8}},
      pos, vector<Feat>{Planeshift, TurnInvisible}, 1, Race::Outsider,
      'g', 'g', Monster::GenieDjinni, nullptr, Item::random_items_chance(20, 1, 1));
    } break;

    case GenieVizier: {
      *this = Monster(23, 19, 14, 14, 15, 17, 1600, 7, 15, 12,
      vector<damage>{{1,8}},
      pos, vector<Feat>{Planeshift, TurnInvisible}, 1, Race::Outsider,
      'g', 'g', Monster::GenieVizier, nullptr, Item::random_items_chance(20, 1, 1));
    } break;

    case GenieEfreeti: {
      *this = Monster(23, 17, 18, 12, 14, 15, 4800, 10, 18, 10,
      vector<damage>{{2,6}},
      pos, vector<Feat>{Planeshift, ScorchingRay, AttacksOnSight}, 1, Race::Outsider,
      'g', 'g', Monster::GenieEfreeti, nullptr, Item::random_items_chance(20, 1, 1));
    } break;

    case GenieJanni: {
      *this = Monster(16, 15, 12, 14, 15, 14, 1200, 6, 18, 10,
      vector<damage>{{1,6}},
      pos, vector<Feat>{}, 1, Race::Outsider,
      'g', 'g', Monster::GenieJanni, nullptr, Item::random_items_chance(20, 1, 1));
    } break;

    case GenieMarid: {
      *this = Monster(23, 19, 18, 14, 15, 16, 6400, 12, 19, 10,
      vector<damage>{{2,6},{2,6}},
      pos, vector<Feat>{Planeshift, TurnInvisible}, 1, Race::Outsider,
      'g', 'g', Monster::GenieMarid, nullptr, Item::random_items_chance(20, 1, 1));
    } break;

    case GenieShaitan: {
      *this = Monster(20, 13, 19, 14, 14, 15, 3200, 10, 19, 10,
      vector<damage>{{1,8}},
      pos, vector<Feat>{Planeshift, MoveThroughStone}, 1, Race::Outsider,
      'g', 'g', Monster::GenieShaitan, nullptr, Item::random_items_chance(20, 1, 1));
    } break;

    case AdultRedDragon: {
      *this = Monster(31, 10, 23, 16, 17, 16, 38400, 17, 30, 12,
      vector<damage>{{2,6},{2,6}},
      pos, vector<Feat>{AttacksOnSight, BreatheConeFireMedium},
      1, Race::Dragon, 'D', 'D', Monster::AdultRedDragon, nullptr,
      Item::random_items(1,4));
    } break;

    case Jabberwock: {
      *this = Monster(37, 20, 33, 12, 29, 26, 819200, 26, 36, 12,
      vector<damage>{{3,6},{3,6}},
      pos, vector<Feat>{AttacksOnSight},
      1, Race::Dragon, 'J', 'J', Monster::Jabberwock, nullptr,
      Item::random_items(1,4));
    } break;

    case NMONSTERS: error("Unknown monster NMONSTERS");
  }

  if (player != nullptr && player->has_ring_with_ability(Ring::AggravateMonsters)) {
    monster_start_running(&pos);
  }
}

void Monster::set_target(Coordinate const* new_target) {
  target = new_target;
}

Coordinate const* Monster::get_target() const {
  return target;
}

char Monster::get_look() const {
  return look;
}

char Monster::get_disguise() const {
  return disguise;
}

void Monster::notice_player() {

  /* Monster can begin chasing after the player if: */
  if (!is_chasing() && is_mean() && !is_held() && !player->is_stealthy() &&
      !os_rand_range(3)) {

    set_target(&player->get_position());
    if (!is_stuck()) {
      set_chasing();
    }
  }
}

string Monster::name(Type subtype) {
  switch (subtype) {
    case Bat:              return "bat";
    case Goblin:           return "goblin";
    case Kobold:           return "kobold";
    case Orc:              return "orc";
    case SkeletonHuman:    return "human skeleton";
    case Hobgoblin:        return "hobgoblin";
    case ZombieHuman:      return "human zombie";
    case Quasit:           return "quasit";
    case Worg:             return "worg";
    case Ogre:             return "ogre";
    case Yeti:             return "yeti";
    case IceMonster:       return "ice monster";
    case Aquator:          return "aquator";
    case Troll:            return "troll";
    case ShadowDemon:      return "shadow demon";
    case Erinyes:          return "erinyes";
    case Nightmare:        return "nightmare";
    case GenieDjinni:      return "djinni";
    case GenieVizier:      return "vizier";
    case GenieEfreeti:     return "efreeti";
    case GenieJanni:       return "janni";
    case GenieMarid:       return "marid";
    case GenieShaitan:     return "shaitan";
    case AdultRedDragon:   return "adult red dragon";
    case Jabberwock:       return "jabberwock";

    case NMONSTERS: error("Unknown monster NMONSTERS");
  }
}

int
monster_save_throw(int which, Monster const* mon)
{

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
  tp->find_new_target();
  if (!tp->is_stuck()) {
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

  /* Get rid of the monster. */
  if (print_message) {
    Game::io->message("you have slain " + monster->get_name());
  }
  monster_remove_from_screen(monster_ptr, true);

  /* Do adjustments if he went up a level */
  player->check_for_level_up();
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
  for (Item* obj : monster->get_pack()) {
    obj->set_position(monster->get_position());
    if (was_killed) {
      weapon_missile_fall(obj, false);
    } else {
      delete obj;
    }
  }
  monster->get_pack().clear();

  Coordinate position = monster->get_position();
  Game::level->set_monster(position, nullptr);
  Game::level->monsters.remove(monster);

  if (monster->is_players_target()) {
    to_death = false;
  }

  delete monster;
  *monster_ptr = nullptr;
}

void
monster_teleport(Monster* monster, Coordinate const* destination)
{
  if (monster == nullptr) {
    error("monster = null");
  }

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
  monster->set_position(new_pos);
  monster->set_not_held();
}

void
monster_do_special_ability(Monster** monster_ptr)
{
  if (monster_ptr == nullptr || *monster_ptr == nullptr) { error("null"); }

  Monster* monster = *monster_ptr;
  if (monster->is_cancelled()) {
    return;
  }

  if (monster->attack_damages_armor()) {
    player->rust_armor();
  }

  if (monster->attack_freezes()) {
    player->set_not_running();
    if (!player_turns_without_action) {
      Game::io->message("you are frozen by the " + monster->get_name());
    }
    player_turns_without_action += os_rand_range(2) + 2;
    if (player_turns_without_action > 50) {
      death(DEATH_ICE);
    }
  }

  if (monster->attack_steals_gold() && player->get_gold() > 0) {
    Game::io->message(monster->get_name() + " steals some of your gold");

    monster_remove_from_screen(monster_ptr, false);
    monster = *monster_ptr = nullptr;

    player->give_gold(-Gold::random_gold_amount());
    if (!player->saving_throw(VS_MAGIC)) {
      player->give_gold(-Gold::random_gold_amount() + Gold::random_gold_amount() +
          Gold::random_gold_amount() + Gold::random_gold_amount());
    }

    if (player->get_gold() < 0) {
      player->give_gold(-player->get_gold());
    }
    return;
  }

  if (monster->attack_steals_item()) {
      Item* steal = player->pack_find_random_item();
      if (steal != nullptr) {
        monster_remove_from_screen(monster_ptr, false);
        monster = *monster_ptr = nullptr;
        player->pack_remove(steal, false, false);
        Game::io->message("your pack feels lighter");
        delete steal;
        return;
      }
  }

  if (monster->attack_drains_strength()) {
    if (!player->saving_throw(VS_POISON)
        && !player->has_ring_with_ability(Ring::SustainStrenght)) {
      player->modify_strength(-1);
      Game::io->message("you feel weaker");
    }
  }

  if (monster->attack_drains_health() && os_rand_range(100) < 30) {
    int fewer = roll(1, 3);
    player->take_damage(fewer);
    player->modify_max_health(-fewer);
    if (player->get_health() <= 0) {
      death(DEATH_NO_HEALTH);
    }
    Game::io->message("you feel weaker");
  }

  if (monster->attack_drains_experience() && os_rand_range(100) < 15) {
    // Death by no level
    if (player->get_level() == 1) {
      death(DEATH_NO_EXP);
    }
    player->lower_level(1);

    int fewer = roll(1, 10);
    player->take_damage(fewer);
    player->modify_max_health(-fewer);
    if (player->get_health() <= 0) {
      death(DEATH_NO_HEALTH);
    }
    Game::io->message("you feel weaker");
  }
}

bool
monster_is_anyone_seen_by_player(void)
{
  for (Monster* mon : Game::level->monsters) {
    if (player->can_see(*mon)) {
      return true;
    }
  }
  return false;
}

void Monster::all_move() {

  // This function needs a manual loop, since monsters can die
  auto it = Game::level->monsters.begin();
  while (it != Game::level->monsters.end()) {
    Monster* mon = *it++;

    int moves = mon->get_moves_this_round();
    for (int i = 0; i < moves; ++i) {
      bool wastarget = mon->is_players_target();
      Coordinate orig_pos = mon->get_position();

      if (!mon->take_turn()) {
        // Monster is dead
        break;
      }

      if (wastarget && !(orig_pos == mon->get_position())) {
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
monster_aggro_all_which_desire_item(Item* item)
{
  for (Monster* mon : Game::level->monsters) {
    if (mon->get_target() == &item->get_position()) {
      mon->set_target(&player->get_position());
    }
  }
}

void
monster_polymorph(Monster* target)
{
  if (target == nullptr) {
    error("null");
  }
  stringstream os;

  Coordinate pos = target->get_position();
  bool was_seen = player->can_see(*target);
  if (was_seen) {
    os << target->get_name();
  }

  // Save some things from old monster
  list<Item*> target_pack = target->get_pack();

  // Generate the new monster
  Monster::Type monster = Monster::random_monster_type();
  bool same_monster = monster == target->get_subtype();

  // TODO: Test this. Important that pack and damage gets copied
  *target = Monster(monster, pos);

  if (player->can_see(*target))
  {
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
  target->get_pack() = target_pack;
}

bool monster_try_breathe_fire_on_player(Monster const& monster) {

  // For dragons, check and see if
  // (a) the hero is on a straight line from it, and
  // (b) that it is within shooting distance, but outside of striking range

  Coordinate mon_coord = monster.get_position();
  Coordinate player_coord = player->get_position();
  int const dragonshot_chance = 5;
  if (monster.get_subtype() == Monster::AdultRedDragon &&
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

void Monster::find_new_target() {
  // Currently, this function only makes it target the player

  if (player->can_see(*this)) {
    set_target(&player->get_position());
  }

  set_target(nullptr);
}

Monster::Type Monster::get_subtype() const {
  return subtype;
}

void Monster::set_disguise(char new_disguise) {
  disguise = new_disguise;
}

Monster::Monster(istream& data) {
  if (!load(data)) {
    error("Malformed monster found");
  }
}

void Monster::save(ostream& data) const {
  Character::save(data);
  Disk::save_tag(TAG_MONSTER, data);

  Disk::save(TAG_MISC, look, data);
  Disk::save(TAG_MISC, disguise, data);
  Disk::save(TAG_MISC, static_cast<int>(subtype), data);
  Disk::save(TAG_MISC, disguise, data);
  Disk::save(TAG_MISC, pack, data);

  Disk::save_tag(TAG_MONSTER, data);
}

bool Monster::load(istream& data) {
  Character::load(data);
  if (!Disk::load_tag(TAG_MONSTER, data) ||

      !Disk::load(TAG_MISC, look, data) ||
      !Disk::load(TAG_MISC, disguise, data) ||
      !Disk::load(TAG_MISC, reinterpret_cast<int&>(subtype), data) ||
      !Disk::load(TAG_MISC, disguise, data) ||
      !Disk::load(TAG_MISC, pack, data) ||

      !Disk::load_tag(TAG_MONSTER, data)) {
    return false;
  }
  return true;
}

list<Item*>& Monster::get_pack() {
  return pack;
}
