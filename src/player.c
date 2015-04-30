#include "rogue.h"

#include "player.h"

coord *
player_get_pos(void)
{
  return &player.t_pos;
}

void
player_set_pos(coord *new_pos)
{
  player.t_pos.x = new_pos->x;
  player.t_pos.y = new_pos->y;
}
