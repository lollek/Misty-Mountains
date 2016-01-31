#include <string>
#include <exception>

using namespace std;

#include "error_handling.h"
#include "game.h"
#include "coordinate.h"
#include "fight.h"
#include "io.h"
#include "level.h"
#include "misc.h"
#include "monster.h"
#include "os.h"
#include "player.h"
#include "things.h"
#include "weapons.h"
#include "death.h"

#include "magic.h"

static bool
magic_bolt_handle_bounces(Coordinate& pos, Coordinate* dir, char* dirtile)
{
  int num_bounces = 0;
recursive_loop:; /* ONLY called by end of function */
  char ch = Game::level->get_type(pos);
  if (ch != VWALL && ch != HWALL && ch != SHADOW)
    return num_bounces != 0;

  /* There are no known bugs with this functions at the moment,
   * but just in case, we'll abort after too many bounces */
  if (num_bounces > 10)
    return true;

  /* Treat shadow as a wall */
  if (ch == SHADOW)
  {
    if (dir->x != 0 && dir->y == 0)
      ch = VWALL;
    else if (dir->y != 0 && dir->x == 0)
      ch = HWALL;
    else
    {
      int y = dir->y < 0 ? pos.y + 1 : pos.y - 1;
      int y_ch;
      if (y >= NUMLINES || y <= 0)
        y_ch = HWALL;
      else
        y_ch = Game::level->get_ch(pos.x, y);

      ch = (y_ch == HWALL || y_ch == VWALL || y_ch == SHADOW)
        ? VWALL : HWALL;
    }
  }
  if (ch == SHADOW) {
    error("ch was still SHADOW");
  }

  /* Handle potential bouncing */
  if (ch == VWALL)
  {
    pos.x -= dir->x;
    dir->x = -dir->x;
  }
  else if (ch == HWALL)
  {
    pos.y -= dir->y;
    dir->y = -dir->y;
  }

  if (*dirtile == BOLT_DIAGDOWN)
    *dirtile = BOLT_DIAGUP;
  else if (*dirtile == BOLT_DIAGUP)
    *dirtile = BOLT_DIAGDOWN;

  /* It's possible for a bolt to bounce directly from one wall to another
   * if you hit a corner, thus, we need to go through everything again. */
  ++num_bounces;
  goto recursive_loop; /* == magic_bolt_handle_bounces(pos,dir,dirtile) */
}

static void
magic_bolt_hit_player(Coordinate* start, string const& missile_name)
{
  if (start == nullptr) {
    error("start coord was null");
  }

  if (!player->saving_throw(VS_MAGIC))
  {
    player->take_damage(roll(6, 6));
    if (player->get_health() <= 0)
    {
      if (start == &player->get_position())
        switch (missile_name[0])
        {
          case 'f': death(DEATH_FLAME);
          case 'i': death(DEATH_ICE);
          default:  death(DEATH_UNKNOWN);
        }
      else
        death(Game::level->get_monster(*start)->get_type());
    }
    io_msg("you are hit by the %s", missile_name.c_str());
  }
  else
    io_msg("the %s whizzes by you", missile_name.c_str());
}

static void
magic_bolt_hit_monster(Monster* mon, Coordinate* start, Coordinate* pos, string const& missile_name)
{
  if (mon == nullptr) {
    error("mon was null");
  } else if (start == nullptr) {
    error("start was null");
  } else if (pos == nullptr) {
    error("pos was null");
  }

  if (!monster_save_throw(VS_MAGIC, mon))
  {
    Weapon bolt(Weapon::SPEAR);
    bolt.set_hit_plus(100);
    bolt.set_damage_plus(0);
    bolt.set_throw_damage({6,6});
    bolt.set_position(*pos);

    if (mon->get_type() == 'D' && missile_name == "flame")
      io_msg("the flame bounces off the dragon");
    else
      fight_against_monster(pos, &bolt, true, &missile_name);
  }
  else if (Game::level->get_type(*pos) != 'M' || mon->t_disguise == 'M')
  {
    if (start == &player->get_position())
      monster_start_running(pos);
    else
    {
      io_msg("the %s whizzes past %s", missile_name.c_str(), mon->get_name().c_str());
    }
  }
}

void
magic_bolt(Coordinate* start, Coordinate* dir, string const& name)
{
  if (start == nullptr) {
    error("start was null");
  } else if (dir == nullptr) {
    error("dir was null");
  }

  char dirtile = '?';
  switch (dir->y + dir->x)
  {
    case 0: dirtile = BOLT_DIAGUP; break;
    case 1: case -1:
      dirtile = (dir->y == 0
          ? BOLT_HORIZONTAL
          : BOLT_VERTICAL);
      break;
    case 2: case -2: dirtile = BOLT_DIAGDOWN; break;
  }

  IO::Attribute color = IO::Attribute::Red;
  if (name == "ice") {
    color = IO::Attribute::Blue;
  }

  Coordinate pos = *start;
  struct charcoord {
    int y;
    int x;
    char ch;
  } spotpos[BOLT_LENGTH];

  /* Special case when someone is standing in a doorway and aims at the wall
   * nearby OR when stainding in a passage and aims at the wall
   * Note that both of those things are really stupid to do */
  char starting_pos = Game::level->get_ch(*start);
  if (starting_pos == DOOR || starting_pos == PASSAGE)
  {
    char first_bounce = Game::level->get_ch(start->x + dir->x, start->y + dir->y);
    bool is_player = *start == player->get_position();
    if (first_bounce == HWALL || first_bounce == VWALL)
    {
      if (is_player)
        for (int i = 0; i < BOLT_LENGTH; ++i)
          magic_bolt_hit_player(start, name);
      else
        for (int i = 0; i < BOLT_LENGTH; ++i)
          magic_bolt_hit_monster(Game::level->get_monster(*start),
                                start, start, name);
      return;
    }
  }

  int i;
  for (i = 0; i < BOLT_LENGTH; ++i)
  {
    pos.y += dir->y;
    pos.x += dir->x;

    if (magic_bolt_handle_bounces(pos, dir, &dirtile))
      io_msg("the %s bounces", name.c_str());

    /* Handle potential hits */
    if (pos == player->get_position())
      magic_bolt_hit_player(start, name);

    Monster* tp = Game::level->get_monster(pos);
    if (tp != nullptr) {
      spotpos[i].ch = static_cast<char>(tp->get_type());
      magic_bolt_hit_monster(tp, start, &pos, name);
    } else {
      spotpos[i].ch = Game::level->get_ch(pos);
    }

    spotpos[i].x = pos.x;
    spotpos[i].y = pos.y;
    Game::io->print(pos.x, pos.y, dirtile, color);
  }

  refresh();
  os_usleep(200000);

  for (int j = i -1; j >= 0; --j)
    Game::io->print_color(spotpos[j].x, spotpos[j].y, spotpos[j].ch);
}


