/*
 * All sorts of miscellaneous routines
 *
 * @(#)misc.c	4.66 (Berkeley) 08/06/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "options.h"
#include "io.h"
#include "armor.h"
#include "pack.h"
#include "daemons.h"
#include "colors.h"
#include "monster.h"
#include "rings.h"
#include "passages.h"
#include "rooms.h"
#include "level.h"
#include "player.h"
#include "os.h"
#include "rogue.h"

#include "misc.h"

/* Return the character appropriate for this space, taking into
 * account whether or not the player is tripping */
static int
trip_ch(int y, int x, int ch)
{
  if (player_is_hallucinating() && after)
    switch (ch)
    {
      case FLOOR: case SHADOW: case PASSAGE: case HWALL: case VWALL: case DOOR:
      case TRAP:
        break;
      default:
        if (y != stairs.y || x != stairs.x || !seen_stairs())
          return rnd_thing();
    }
  return ch;
}

int
rnd(int range)
{
  return range == 0 ? 0 : rand_r(&seed) % range;
}

int
roll(int number, int sides)
{
  int dtotal = 0;

  while (number--)
    dtotal += rnd(sides)+1;
  return dtotal;
}

void
look(bool wakeup)
{
  int x, y;
  coord *player_pos = player_get_pos();
  PLACE *pp = INDEX(player_pos->y, player_pos->x);
  char pch = pp->p_ch;
  char pfl = pp->p_flags;
  struct room *rp = player_get_room();
  int passcount = 0;
  int sumhero = 0, diffhero = 0;

  if (!same_coords(oldpos, *player_pos))
  {
    erase_lamp(&oldpos, oldrp);
    oldpos = *player_pos;
    oldrp = rp;
  }

  if (door_stop && !firstmove && running)
  {
    sumhero = player_pos->y + player_pos->x;
    diffhero = player_pos->y - player_pos->x;
  }

  for (y = player_pos->y - 1; y <= player_pos->y + 1; y++)
    if (y > 0 && y < NUMLINES - 1)
      for (x = player_pos->x -1; x <= player_pos->x + 1; x++)
      {
        char ch;
        const char *fp;
        THING *tp;

        if (x < 0 || x >= NUMCOLS)
          continue;

        if (!player_is_blind() && y == player_pos->y && x == player_pos->x)
            continue;

        pp = INDEX(y, x);
        ch = pp->p_ch;

        if (ch == SHADOW)  /* nothing need be done with a ' ' */
          continue;

        fp = &pp->p_flags;

        if (pch != DOOR && ch != DOOR && (pfl & F_PASS) != (*fp & F_PASS))
            continue;

        if (((*fp & F_PASS) || ch == DOOR) &&
            ((pfl & F_PASS) || pch == DOOR))
        {
          if (player_pos->x != x && player_pos->y != y &&
              !step_ok(chat(y, player_pos->x)) && !step_ok(chat(player_pos->y, x)))
            continue;
        }

        if ((tp = pp->p_monst) == NULL)
          ch = trip_ch(y, x, ch);
        else
        {
          if (player_can_sense_monsters() && monster_is_invisible(tp))
          {
            if (door_stop && !firstmove)
              running = false;
            continue;
          }
          else
          {
            if (wakeup)
              monster_notice_player(y, x);
            if (see_monst(tp))
            {
              ch = player_is_hallucinating()
                ? rnd(26) + 'A'
                : tp->t_disguise;
            }
          }
        }

        if (player_is_blind() && (y != player_pos->y || x != player_pos->x))
          continue;

        move(y, x);

        if ((player_get_room()->r_flags & ISDARK) && !see_floor && ch == FLOOR)
          ch = SHADOW;

        if (tp != NULL || ch != incch())
          addcch(ch);

        if (door_stop && !firstmove && running)
        {
          switch (runch)
          {
            case 'h': if (x == player_pos->x + 1)
                        continue;
                      break;
            case 'j': if (y == player_pos->y - 1)
                        continue;
                      break;
            case 'k': if (y == player_pos->y + 1)
                        continue;
                      break;
            case 'l': if (x == player_pos->x - 1)
                        continue;
                      break;
            case 'y': if ((y + x) - sumhero >= 1)
                        continue;
                      break;
            case 'u': if ((y - x) - diffhero >= 1)
                        continue;
                      break;
            case 'n': if ((y + x) - sumhero <= -1)
                        continue;
                      break;
            case 'b': if ((y - x) - diffhero <= -1)
                        continue;
                      break;
          }
          switch (ch)
          {
            case DOOR:    if (x == player_pos->x || y == player_pos->y)
                            running = false;
                          break;
            case PASSAGE: if (x == player_pos->x || y == player_pos->y)
                            passcount++;
                          break;
            case FLOOR: case VWALL: case HWALL: case SHADOW:
              break;
            default: running = false;
                     break;
          }
        }
      }
  if (door_stop && !firstmove && passcount > 1)
    running = false;
  if (!running || !jump)
    mvaddcch(player_pos->y, player_pos->x, PLAYER);
}

void
erase_lamp(coord *pos, struct room *rp)
{
  coord *player_pos = player_get_pos();
  int y, x;

  if (!(see_floor && (rp->r_flags & (ISGONE|ISDARK)) == ISDARK
        && !player_is_blind()))
    return;

  for (x = pos->x -1; x <= pos->x +1; x++)
    for (y = pos->y -1; y <= pos->y +1; y++)
    {
      if (y == player_pos->y && x == player_pos->x)
        continue;

      move(y, x);
      if (incch() == FLOOR)
        addcch(SHADOW);
    }
}

bool
show_floor(void)
{
  return
    ((player_get_room()->r_flags & (ISGONE|ISDARK)) == ISDARK
     && !player_is_blind())
    ? see_floor
    : true;
}

THING *
find_obj(int y, int x)
{
  THING *obj;

  for (obj = lvl_obj; obj != NULL; obj = obj->l_next)
    if (obj->o_pos.y == y && obj->o_pos.x == x)
      return obj;

  /* It should have returned by now */
  msg("DEBUG: Non-object %d,%d", y, x);
  return NULL;
}

bool
eat(void)
{
    THING *obj = pack_get_item("eat", FOOD);

    if (obj == NULL)
      return false;

    if (obj->o_type != FOOD)
    {
      msg("that's inedible!");
      return false;
    }

    food_left = (food_left > 0 ? food_left : 0) + HUNGERTIME - 200 + rnd(400);
    if (food_left > STOMACHSIZE)
      food_left = STOMACHSIZE;

    hungry_state = 0;

    if (obj->o_which == 1)
      msg("my, that was a yummy fruit");
    else
      if (rnd(100) > 70)
      {
        player_earn_exp(1);
        msg("%s, this food tastes awful",
            player_is_hallucinating() ? "bummer" : "yuk");
        player_check_for_level_up();
      }
      else
        msg("%s, that tasted good",
            player_is_hallucinating() ? "oh, wow" : "yum");

    pack_remove(obj, false, false);
    return true;
}

void
aggravate(void)
{
  THING *mp;
  for (mp = mlist; mp != NULL; mp = mp->l_next)
    monster_start_running(&mp->t_pos);
}

const char *
vowelstr(const char *str)
{
  switch (str[0])
  {
    case 'a': case 'A':
    case 'e': case 'E':
    case 'i': case 'I':
    case 'o': case 'O':
    case 'u': case 'U':
      return "n";
    default:
      return "";
  }
}

const coord *
get_dir(void)
{
  const char *prompt = terse ? "direction? " : "which direction? ";
  bool gotit;
  static coord delta;
  static coord last_delt= {0,0};

  if (again && last_dir != '\0')
  {
    delta.y = last_delt.y;
    delta.x = last_delt.x;
    dir_ch = last_dir;
  }
  else
  {
    msg(prompt);

    do
    {
      gotit = true;
      switch (dir_ch = readchar())
      {
        case 'h': case'H': delta.y =  0; delta.x = -1; break;
        case 'j': case'J': delta.y =  1; delta.x =  0; break;
        case 'k': case'K': delta.y = -1; delta.x =  0; break;
        case 'l': case'L': delta.y =  0; delta.x =  1; break;
        case 'y': case'Y': delta.y = -1; delta.x = -1; break;
        case 'u': case'U': delta.y = -1; delta.x =  1; break;
        case 'b': case'B': delta.y =  1; delta.x = -1; break;
        case 'n': case'N': delta.y =  1; delta.x =  1; break;
        case KEY_ESCAPE:
          last_dir = '\0';
          reset_last();
          msg("");
          return NULL;

        default:
          mpos = 0;
          msg(prompt);
          gotit = false;
          break;
      }
    } while (!gotit);

    if (isupper(dir_ch))
      dir_ch = (char) tolower(dir_ch);

    last_dir = dir_ch;
    last_delt.y = delta.y;
    last_delt.x = delta.x;
  }
  if (player_is_confused() && rnd(5) == 0)
    do
    {
      delta.y = rnd(3) - 1;
      delta.x = rnd(3) - 1;
    } while (delta.y == 0 && delta.x == 0);

  mpos = 0;
  return &delta;
}

int
sign(int nm)
{
  if (nm < 0)
    return -1;
  else
    return (nm > 0);
}

int
spread(int nm)
{
  return nm - nm / 20 + rnd(nm / 10);
}

void
call_it(const char *what, struct obj_info *info)
{
  if (info->oi_know)
  {
    if (info->oi_guess != NULL)
    {
      free(info->oi_guess);
      info->oi_guess = NULL;
    }
  }

  else if (!info->oi_guess)
  {
    char tmpbuf[MAXSTR] = { '\0' };
    msg("what do you want to name the %s? ", what);
    if (readstr(tmpbuf) == 0)
    {
      if (info->oi_guess != NULL)
        free(info->oi_guess);
      info->oi_guess = malloc((unsigned int) strlen(tmpbuf) + 1);
      strcpy(info->oi_guess, tmpbuf);
    }
  }
}

char
rnd_thing(void)
{
  char thing_list[] = {
    POTION, SCROLL, RING, STICK, FOOD, WEAPON, ARMOR, STAIRS, GOLD, AMULET
  };
  int i;

  if (level >= AMULETLEVEL)
    i = rnd(sizeof thing_list / sizeof (char));
  else
    i = rnd(sizeof thing_list / sizeof (char) - 1);
  return thing_list[i];
}

bool
is_magic(THING *obj)
{
  switch (obj->o_type)
  {
    case ARMOR:
      return (bool)(obj->o_flags & ISPROT) ||
             obj->o_arm != armor_ac(obj->o_which);

    case WEAPON:
      return obj->o_hplus != 0 || obj->o_dplus != 0;

    case POTION: case SCROLL: case STICK: case RING: case AMULET:
      return true;
  }
  return false;
}

bool
seen_stairs(void)
{
  THING *tp = moat(stairs.y, stairs.x);

  move(stairs.y, stairs.x);
  if (incch() == STAIRS)  /* it's on the map */
    return true;
  if (same_coords(*player_get_pos(), stairs)) /* It's under him */
    return true;

  /* if a monster is on the stairs, this gets hairy */
  if (tp != NULL)
  {
    if (see_monst(tp) && on(*tp, ISRUN)) /* if it's visible and awake */
      return true;                       /* it must have moved there */

    if (player_can_sense_monsters()      /* if she can detect monster */
        && tp->t_oldch == STAIRS)        /* and there once were stairs */
      return true;                       /* it must have moved there */
  }
  return false;
}

void
invis_on(void)
{
  THING *mp;

  player_add_true_sight(true);
  for (mp = mlist; mp != NULL; mp = mp->l_next)
    if (monster_is_invisible(mp) && see_monst(mp) && !player_is_hallucinating())
      mvaddcch(mp->t_pos.y, mp->t_pos.x, mp->t_disguise);
}


void
strucpy(char *s1, const char *s2, int len)
{
    if (len > MAXINP)
	len = MAXINP;
    while (len--)
    {
	if (isprint(*s2) || *s2 == ' ')
	    *s1++ = *s2;
	s2++;
    }
    *s1 = '\0';
}

void
waste_time(int rounds)
{
  while (rounds--)
  {
    daemon_run_before();
    daemon_run_after();
  }
}

void
set_oldch(THING *tp, coord *cp)
{
  char sch = tp->t_oldch;

  if (same_coords(tp->t_pos, *cp))
    return;

  tp->t_oldch = mvincch(cp->y, cp->x);
  if (!player_is_blind())
  {
    if ((sch == FLOOR || tp->t_oldch == FLOOR) &&
        (tp->t_room->r_flags & ISDARK))
      tp->t_oldch = SHADOW;
    else if (dist_cp(cp, player_get_pos()) <= LAMPDIST && see_floor)
      tp->t_oldch = chat(cp->y, cp->x);
  }
}

bool
see_monst(THING *mp)
{
  coord *player_pos = player_get_pos();
  int y = mp->t_pos.y;
  int x = mp->t_pos.x;

  if (player_is_blind() ||
      (monster_is_invisible(mp) && !player_has_true_sight()))
    return false;

  if (dist(y, x, player_pos->y, player_pos->x) < LAMPDIST)
  {
    if (y != player_pos->y && x != player_pos->x &&
        !step_ok(chat(y, player_pos->x)) && !step_ok(chat(player_pos->y, x)))
      return false;
    return true;
  }

  if (mp->t_room != player_get_room())
    return false;
  return ((bool)!(mp->t_room->r_flags & ISDARK));
}

struct room *
roomin(coord *cp)
{
  char *fp = &flat(cp->y, cp->x);
  struct room *rp;

  if (*fp & F_PASS)
    return &passages[*fp & F_PNUM];

  for (rp = rooms; rp < &rooms[MAXROOMS]; rp++)
    if (cp->x <= rp->r_pos.x + rp->r_max.x && rp->r_pos.x <= cp->x
        && cp->y <= rp->r_pos.y + rp->r_max.y && rp->r_pos.y <= cp->y)
      return rp;

  msg("in some bizarre place (%d, %d)", cp->y, cp->x);
  assert(0);
  return NULL;
}

bool
diag_ok(coord *sp, coord *ep)
{
  if (ep->x < 0 || ep->x >= NUMCOLS || ep->y <= 0 || ep->y >= NUMLINES - 1)
    return false;
  if (ep->x == sp->x || ep->y == sp->y)
    return true;
  return (bool)(step_ok(chat(ep->y, sp->x)) && step_ok(chat(sp->y, ep->x)));
}

bool
cansee(int y, int x)
{
    coord *player_pos = player_get_pos();
    struct room *rer;
    static coord tp;

    if (player_is_blind())
	return false;
    if (dist(y, x, player_pos->y, player_pos->x) < LAMPDIST)
    {
	if (flat(y, x) & F_PASS)
	    if (y != player_pos->y && x != player_pos->x &&
		!step_ok(chat(y, player_pos->x))
                && !step_ok(chat(player_pos->y, x)))
		    return false;
	return true;
    }
    /*
     * We can only see if the hero in the same room as
     * the coordinate and the room is lit or if it is close.
     */
    tp.y = y;
    tp.x = x;
    return (bool)((rer = roomin(&tp)) == player_get_room() && !(rer->r_flags & ISDARK));
}

int
dist(int y1, int x1, int y2, int x2)
{
    return ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

char *
set_mname(THING *tp)
{
    int ch;
    char *mname;
    static char tbuf[MAXSTR] = { 't', 'h', 'e', ' ' };

    if (!see_monst(tp) && !player_can_sense_monsters())
	return (terse ? "it" : "something");
    else if (player_is_hallucinating())
    {
	move(tp->t_pos.y, tp->t_pos.x);
	ch = incch();
	if (!isupper(ch))
	    ch = rnd(26);
	else
	    ch -= 'A';
	mname = monsters[ch].m_name;
    }
    else
	mname = monsters[tp->t_type - 'A'].m_name;
    strcpy(&tbuf[4], mname);
    return tbuf;
}

const char *
pick_color(const char *col)
{
  return player_is_hallucinating() ? colors_random() : col;
}

char
floor_ch(void)
{
  if (player_get_room()->r_flags & ISGONE)
    return PASSAGE;
  return show_floor() ? FLOOR : SHADOW;
}

char
floor_at(void)
{
  coord *player_pos = player_get_pos();
  char ch = chat(player_pos->y, player_pos->x);
  return ch == FLOOR ? floor_ch() : ch;
}


void
reset_last(void)
{
  last_comm = l_last_comm;
  last_dir = l_last_dir;
  pack_reset_last_picked_item();
}

bool
player_has_ring_with_ability(int ability)
{
  int i;
  for (i = 0; i < RING_SLOTS_SIZE; ++i)
  {
    THING *ring = pack_equipped_item(ring_slots[i]);
    if (ring != NULL && ring->o_which == ability)
      return true;
  }
  return false;
}
