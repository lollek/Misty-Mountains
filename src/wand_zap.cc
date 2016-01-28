#include <vector>

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

  while (step_ok(Game::level->get_type(*x, *y))) {
    *y += dy;
    *x += dx;
  }

  return Game::level->get_monster(*x, *y);
}

static void wand_spell_light(void)
{
  if (player->get_room()->r_flags & ISGONE) {
    io_msg("the corridor glows and then fades");
    return;
  }

  player->get_room()->r_flags &= ~ISDARK;
  room_enter(player->get_position());
  io_msg("the rooms is lit by a shimmering %s light",
          player->is_hallucinating() ? color_random().c_str() : "blue");
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
  struct room *corp = Game::level->get_ch(player_pos) == DOOR
    ? Game::level->get_passage(player_pos)
    : nullptr;

  for (Monster* monster : Game::level->monsters) {

    // Monsters in same room
    if (monster->get_room() == player->get_room() ||
        monster->get_room() == corp) {
      drainee.push_back(monster);
    }

    // Nearby monster in same passage
    else if (in_passage && Game::level->get_ch(monster->get_position()) == DOOR &&
        Game::level->get_passage(monster->get_position()) == player->get_room()) {
      drainee.push_back(monster);
    }
  }

  // No monster found -> return
  if (drainee.empty()) {
    io_msg("you have a tingling feeling");
    return;
  }

  int damage = player->get_health() / 2;
  player->take_damage(damage);
  io_msg("You feel an intense pain");

  // Now zot all of the monsters
  for (Monster* monster : drainee) {
    monster->take_damage(damage);

    if (monster->get_health() < 0) {
      monster_on_death(monster, monster_seen_by_player(monster));

    } else {
      monster_start_running(&monster->get_position());
      io_msg("%s screams in pain", monster->get_name().c_str());
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
  Item bolt{};
  bolt.o_type    = '*';
  bolt.o_hplus   = 100;
  bolt.o_dplus   = 1;
  bolt.o_flags   = ISMISL;
  bolt.o_damage  = {0, 0};
  bolt.o_hurldmg = {1, 4};

  Item* weapon = pack_equipped_item(EQUIPMENT_RHAND);
  if (weapon != nullptr) {
    bolt.o_launch = weapon->o_which;
  }

  io_missile_motion(&bolt, dy, dx);

  Monster* target = Game::level->get_monster(bolt.get_pos());
  if (target == nullptr) {
    io_msg("the missle vanishes with a puff of smoke");

  } else if (monster_save_throw(VS_MAGIC, target)) {
    io_msg("the missle missed the %s", target->get_name().c_str());

  } else {
    fight_against_monster(&bolt.get_pos(), &bolt, true);
  }
}



bool
wand_zap(void)
{
  Coordinate const* dir = get_dir();
  if (dir == nullptr) {
    return false;
  }

  Item* obj = pack_get_item("zap with", STICK);
  if (obj == nullptr)
    return false;

  else if (obj->o_type != STICK) {
    io_msg("you can't zap with that!");
    return false;

  } else if (obj->o_charges == 0) {
    io_msg("nothing happens");
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
        io_msg("you are too weak to use it");
        return true;
      }
    } break;

    case Wand::INVIS: {
        Coordinate c;
        Monster* tp = wand_find_target(&c.y, &c.x, dir->y, dir->x);
        if (tp != nullptr) {
          tp->set_invisible();
        } else {
          io_msg("You did not hit anything");
        }
      } break;

    case Wand::POLYMORPH:
      {
        Coordinate c;
        Monster* tp = wand_find_target(&c.y, &c.x, dir->y, dir->x);
        if (tp != nullptr)
          wand_spell_polymorph(*tp);
        else
          io_msg("You did not hit anything");
      } break;

    case Wand::CANCEL: {
        Coordinate c;
        Monster* tp = wand_find_target(&c.y, &c.x, dir->y, dir->x);
        if (tp != nullptr) {
          wand_spell_cancel(*tp);
        } else {
          io_msg("You did not hit anything");
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
          io_msg("You did not hit anything");
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
          io_msg("%s became faster", tp->get_name().c_str());

        } else {
          io_msg("You did not hit anything");
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
          io_msg("%s became slower", tp->get_name().c_str());

        } else {
          io_msg("You did not hit anything");
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
      io_msg("You are usure if anything happened");
    } break;

    case Wand::NWANDS: error("Unknown type NWANDS");
  }

    obj->o_charges--;
    return true;
}

