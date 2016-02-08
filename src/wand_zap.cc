#include <vector>

#include "weapons.h"
#include "error_handling.h"
#include "magic.h"
#include "monster.h"
#include "player.h"
#include "game.h"
#include "colors.h"
#include "pack.h"
#include "fight.h"
#include "misc.h"

#include "wand.h"

using namespace std;

// "walk" in the zap direction until we find a target
static Monster* wand_find_target(int* y, int* x, int dy, int dx) {

  *y = player->get_position().y;
  *x = player->get_position().x;

  while (Game::level->can_step(*x, *y)) {
    *y += dy;
    *x += dx;
  }

  return Game::level->get_monster(*x, *y);
}

static void wand_spell_light(void)
{
  if (player->get_room()->r_flags & ISGONE) {
    Game::io->message("the corridor glows and then fades");
    return;
  }

  player->get_room()->r_flags &= ~ISDARK;
  room_enter(player->get_position());
  Game::io->message("the rooms is lit by a shimmering blue light");
}

// take away 1/2 of hero's hit points, then take it away
// evenly from the monsters in the room (or next to hero
// if he is in a passage)
static void
wand_spell_drain_health(void) {

  // Add nearby monsters to a list
  vector<Monster*> drainee;
  bool in_passage = player->get_room()->r_flags & ISGONE;
  Coordinate const& player_pos = player->get_position();
  struct room *corp = Game::level->get_tile(player_pos) == Tile::Door
    ? Game::level->get_passage(player_pos)
    : nullptr;

  for (Monster* monster : Game::level->monsters) {

    // Monsters in same room
    if (monster->get_room() == player->get_room() ||
        monster->get_room() == corp) {
      drainee.push_back(monster);
    }

    // Nearby monster in same passage
    else if (in_passage && Game::level->get_tile(monster->get_position()) == Tile::Door &&
        Game::level->get_passage(monster->get_position()) == player->get_room()) {
      drainee.push_back(monster);
    }
  }

  // No monster found -> return
  if (drainee.empty()) {
    Game::io->message("you have a tingling feeling");
    return;
  }

  int damage = player->get_health() / 2;
  player->take_damage(damage);
  Game::io->message("You feel an intense pain");

  // Now zot all of the monsters
  // Must use manual loop here, since monsters can be deleted
  auto it = drainee.begin();
  while (it != drainee.end()) {
    Monster* monster = *it++;
    monster->take_damage(damage);

    if (monster->get_health() < 0) {
      monster_on_death(&monster, monster_seen_by_player(monster));

    } else {
      monster_start_running(&monster->get_position());
      Game::io->message(monster->get_name() + " screams in pain");
    }
  }
}

static void
wand_spell_polymorph(Monster& target)
{
  monster_polymorph(&target);
  if (monster_seen_by_player(&target)) {
    Wand::set_known(Wand::Type::POLYMORPH);
  }
}

static void
wand_spell_cancel(Monster& target)
{
  if (target.get_type() == 'F') {
    player->set_not_held();
  }

  target.set_cancelled();
  target.set_not_invisible();
  target.remove_confusing_attack();

  target.t_disguise = static_cast<char>(target.get_type());
  if (monster_seen_by_player(&target)) {
    Game::io->print_color(target.get_position().x, target.get_position().y,
        target.t_disguise);
  }
}

static void
wand_spell_magic_missile(int dy, int dx)
{
  Weapon bolt(Weapon::DAGGER);
  bolt.o_type    = '*';
  bolt.set_hit_plus(100);
  bolt.set_damage_plus(1);
  bolt.o_flags   = ISMISL;
  bolt.set_attack_damage({0, 0});
  bolt.set_throw_damage({1, 4});

  Item* weapon = pack_equipped_item(EQUIPMENT_RHAND);
  if (weapon != nullptr) {
    bolt.o_launch = weapon->o_which;
  }

  io_missile_motion(&bolt, dy, dx);

  Monster* target = Game::level->get_monster(bolt.get_position());
  if (target == nullptr) {
    Game::io->message("the missle vanishes with a puff of smoke");

  } else if (monster_save_throw(VS_MAGIC, target)) {
    Game::io->message("the missle missed the " + target->get_name());

  } else {
    fight_against_monster(&bolt.get_position(), &bolt, true);
  }
}



bool
wand_zap(void)
{
  Coordinate const* dir = get_dir();
  if (dir == nullptr) {
    return false;
  }

  Item* obj = pack_get_item("zap with", IO::Wand);
  Wand* wand = dynamic_cast<Wand*>(obj);
  if (obj == nullptr)
    return false;

  else if (obj->o_type != IO::Wand || wand == nullptr) {
    Game::io->message("you can't zap with that!");
    return false;

  } else if (wand->get_charges() == 0) {
    Game::io->message("nothing happens");
    return true;
  }

  Wand::Type subtype = static_cast<Wand::Type>(obj->o_which);
  switch (subtype)
  {

    case Wand::LIGHT: {
      Wand::set_known(subtype);
      wand_spell_light();
    } break;

    case Wand::DRAIN: {
      if (player->get_health() > 1) {
        wand_spell_drain_health();

      } else {
        Game::io->message("you are too weak to use it");
        return true;
      }
    } break;

    case Wand::INVIS: {
        Coordinate c;
        Monster* tp = wand_find_target(&c.y, &c.x, dir->y, dir->x);
        if (tp != nullptr) {
          tp->set_invisible();
        } else {
          Game::io->message("You did not hit anything");
        }
      } break;

    case Wand::POLYMORPH:
      {
        Coordinate c;
        Monster* tp = wand_find_target(&c.y, &c.x, dir->y, dir->x);
        if (tp != nullptr)
          wand_spell_polymorph(*tp);
        else
          Game::io->message("You did not hit anything");
      } break;

    case Wand::CANCEL: {
        Coordinate c;
        Monster* tp = wand_find_target(&c.y, &c.x, dir->y, dir->x);
        if (tp != nullptr) {
          wand_spell_cancel(*tp);
        } else {
          Game::io->message("You did not hit anything");
        }
      } break;

    case Wand::TELAWAY: {
      Wand::set_known(subtype);
      player->teleport(nullptr);
    } break;

    case Wand::TELTO: {
        Wand::set_known(subtype);
        int x;
        int y;
        Coordinate delta = *dir;
        Monster* tp = wand_find_target(&y, &x, delta.y, delta.x);
        if (tp != nullptr)
        {
          Coordinate new_pos;
          new_pos.y = y - delta.y;
          new_pos.x = x - delta.x;

          tp->set_target(&player->get_position());
          tp->set_chasing();

          player->teleport(&new_pos);
        }
        else
          Game::io->message("You did not hit anything");
      } break;

    case Wand::MISSILE: {
      Wand::set_known(subtype);
      wand_spell_magic_missile(dir->y, dir->x);
    } break;

    case Wand::HASTE_M: {
        Coordinate c;
        Monster* tp = wand_find_target(&c.y, &c.x, dir->y, dir->x);
        if (tp != nullptr) {
          if (tp->is_slowed()) {
            tp->set_not_slowed();
          } else {
            tp->set_hasted();
          }
          monster_start_running(&c);
          Game::io->message(tp->get_name() + " became faster");

        } else {
          Game::io->message("You did not hit anything");
        }
      } break;

    case Wand::SLOW_M: {
        Coordinate c;
        Monster* tp = wand_find_target(&c.y, &c.x, dir->y, dir->x);
        if (tp != nullptr) {

          if (tp->is_hasted()) {
            tp->set_not_hasted();
          } else {
            tp->set_slowed();
          }
          tp->t_turn = true;
          monster_start_running(&c);
          Game::io->message(tp->get_name() + " became slower");

        } else {
          Game::io->message("You did not hit anything");
        }
      } break;

    case Wand::ELECT: {
      Wand::set_known(subtype);
      Coordinate delta = *dir;
      Coordinate coord = player->get_position();
      magic_bolt(&coord, &delta, "bolt");
    } break;

    case Wand::FIRE: {
      Wand::set_known(subtype);
      Coordinate coord = player->get_position();
      Coordinate delta = *dir;
      magic_bolt(&coord, &delta, "flame");
    } break;

    case Wand::COLD: {
      Wand::set_known(subtype);
      Coordinate coord = player->get_position();
      Coordinate delta = *dir;
      magic_bolt(&coord, &delta, "ice");
    } break;

    case Wand::NOP: {
      Game::io->message("You are usure if anything happened");
    } break;

    case Wand::NWANDS: error("Unknown type NWANDS");
  }

    wand->modify_charges(-1);
    return true;
}

