#include <assert.h>
#include <string.h>

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
magic_bolt_handle_bounces(coord* pos, coord* dir, tile* dirtile)
{
  int num_bounces = 0;
recursive_loop:; /* ONLY called by end of function */
  char ch = level_get_type(pos->y, pos->x);
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
      int y = dir->y < 0 ? pos->y + 1 : pos->y - 1;
      int y_ch;
      if (y >= NUMLINES || y <= 0)
        y_ch = HWALL;
      else
        y_ch = level_get_ch(y, pos->x);

      ch = (y_ch == HWALL || y_ch == VWALL || y_ch == SHADOW)
        ? VWALL : HWALL;
    }
  }
  assert(ch != SHADOW);

  /* Handle potential bouncing */
  if (ch == VWALL)
  {
    pos->x -= dir->x;
    dir->x = -dir->x;
  }
  else if (ch == HWALL)
  {
    pos->y -= dir->y;
    dir->y = -dir->y;
  }

  if (*dirtile == TILE_BOLT_DIAGDOWN)
    *dirtile = TILE_BOLT_DIAGUP;
  else if (*dirtile == TILE_BOLT_DIAGUP)
    *dirtile = TILE_BOLT_DIAGDOWN;

  /* It's possible for a bolt to bounce directly from one wall to another
   * if you hit a corner, thus, we need to go through everything again. */
  ++num_bounces;
  goto recursive_loop; /* == magic_bolt_handle_bounces(pos,dir,dirtile) */
}

static void
magic_bolt_hit_player(coord* start, char const* missile_name)
{
  if (!player_save_throw(VS_MAGIC))
  {
    player_lose_health(roll(6, 6));
    if (player_get_health() <= 0)
    {
      if (start == player_get_pos())
        switch (missile_name[0])
        {
          case 'f': death(DEATH_FLAME);
          case 'i': death(DEATH_ICE);
          default:  death(DEATH_UNKNOWN);
        }
      else
        death(level_get_monster(start->y, start->x)->t.t_type);
    }
    io_msg("you are hit by the %s", missile_name);
  }
  else
    io_msg("the %s whizzes by you", missile_name);
}

static void
magic_bolt_hit_monster(THING* mon, coord* start, coord* pos, char* missile_name)
{
  mon->t.t_oldch = level_get_ch(pos->y, pos->x);
  if (!monster_save_throw(VS_MAGIC, &mon->t))
  {
    THING bolt;
    memset(&bolt, 0, sizeof(bolt));
    bolt.o.o_type       = WEAPON;
    bolt.o.o_which      = FLAME;
    bolt.o.o_hplus      = 100;
    bolt.o.o_dplus      = 0;
    bolt.o.o_pos        = *pos;
    bolt.o.o_flags     |= ISMISL;
    bolt.o.o_launch     = -1;
    bolt.o.o_hurldmg    = (struct damage){6,6};
    weapon_info[FLAME].oi_name = missile_name;

    if (mon->t.t_type == 'D' && strcmp(missile_name, "flame") == 0)
      io_msg("the flame bounces off the dragon");
    else
      fight_against_monster(pos, &bolt, true);
  }
  else if (level_get_type(pos->y, pos->x) != 'M' || mon->t.t_disguise == 'M')
  {
    if (start == player_get_pos())
      monster_start_running(pos);
    else
    {
      char buf[MAXSTR];
      io_msg("the %s whizzes past %s", missile_name, monster_name(&mon->t, buf));
    }
  }
}

void
magic_bolt(coord* start, coord* dir, char* name)
{
  tile dirtile = TILE_ERROR;
  switch (dir->y + dir->x)
  {
    case 0: dirtile = TILE_BOLT_DIAGUP; break;
    case 1: case -1:
      dirtile = (dir->y == 0
          ? TILE_BOLT_HORIZONTAL
          : TILE_BOLT_VERTICAL);
      break;
    case 2: case -2: dirtile = TILE_BOLT_DIAGDOWN; break;
  }
  assert (dirtile != TILE_ERROR);

  enum attribute bolt_type = ATTR_FIRE;

  if (!strcmp(name, "ice"))
    bolt_type = ATTR_ICE;
  else if (!strcmp(name, "flame"))
    bolt_type = ATTR_FIRE;

  coord pos = *start;
  struct charcoord {
    int y;
    int x;
    char ch;
  } spotpos[BOLT_LENGTH];

  /* Special case when someone is standing in a doorway and aims at the wall
   * nearby OR when stainding in a passage and aims at the wall
   * Note that both of those things are really stupid to do */
  char starting_pos = level_get_ch(start->y, start->x);
  if (starting_pos == DOOR || starting_pos == PASSAGE)
  {
    char first_bounce = level_get_ch(start->y + dir->y, start->x + dir->x);
    bool is_player = coord_same(start, player_get_pos());
    if (first_bounce == HWALL || first_bounce == VWALL)
    {
      if (is_player)
        for (int i = 0; i < BOLT_LENGTH; ++i)
          magic_bolt_hit_player(start, name);
      else
        for (int i = 0; i < BOLT_LENGTH; ++i)
          magic_bolt_hit_monster(level_get_monster(start->y, start->x),
                                start, start, name);
      return;
    }
  }

  int i;
  for (i = 0; i < BOLT_LENGTH; ++i)
  {
    pos.y += dir->y;
    pos.x += dir->x;

    if (magic_bolt_handle_bounces(&pos, dir, &dirtile))
      io_msg("the %s bounces", name);

    /* Handle potential hits */
    if (coord_same(&pos, player_get_pos()))
      magic_bolt_hit_player(start, name);

    THING* tp = level_get_monster(pos.y, pos.x);
    if (tp != NULL)
      magic_bolt_hit_monster(tp, start, &pos, name);

    spotpos[i].x = pos.x;
    spotpos[i].y = pos.y;
    spotpos[i].ch = (char) mvincch(pos.y, pos.x);
    io_addch(dirtile, bolt_type);
    refresh();
  }

  os_usleep(200000);

  for (int j = i -1; j >= 0; --j)
    mvaddcch(spotpos[j].y, spotpos[j].x, (chtype) spotpos[j].ch);
}


