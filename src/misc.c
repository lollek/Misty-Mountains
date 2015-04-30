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

#include "status_effects.h"
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
#include "rogue.h"

#include "misc.h"

/* Return the character appropriate for this space, taking into
 * account whether or not the player is tripping */
static int
trip_ch(int y, int x, int ch)
{
  if (is_hallucinating(&player) && after)
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
  PLACE *pp = INDEX(hero.y, hero.x);
  char pch = pp->p_ch;
  char pfl = pp->p_flags;
  struct room *rp = proom;
  int passcount = 0;
  int sumhero = 0, diffhero = 0;

  if (!same_coords(oldpos, hero))
  {
    erase_lamp(&oldpos, oldrp);
    oldpos = hero;
    oldrp = rp;
  }

  if (door_stop && !firstmove && running)
  {
    sumhero = hero.y + hero.x;
    diffhero = hero.y - hero.x;
  }

  for (y = hero.y - 1; y <= hero.y + 1; y++)
    if (y > 0 && y < NUMLINES - 1)
      for (x = hero.x -1; x <= hero.x + 1; x++)
      {
        char ch;
        const char *fp;
        THING *tp;

        if (x < 0 || x >= NUMCOLS)
          continue;

        if (!is_blind(&player) && y == hero.y && x == hero.x)
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
          if (hero.x != x && hero.y != y &&
              !step_ok(chat(y, hero.x)) && !step_ok(chat(hero.y, x)))
            continue;
        }

        if ((tp = pp->p_monst) == NULL)
          ch = trip_ch(y, x, ch);
        else
        {
          if (on(player, SEEMONST) && is_invisible(tp))
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
              if (is_hallucinating(&player))
                ch = rnd(26) + 'A';
              else
                ch = tp->t_disguise;
            }
          }
        }

        if (is_blind(&player) && (y != hero.y || x != hero.x))
          continue;

        move(y, x);

        if ((proom->r_flags & ISDARK) && !see_floor && ch == FLOOR)
          ch = SHADOW;

        if (tp != NULL || ch != incch())
          addcch(ch);

        if (door_stop && !firstmove && running)
        {
          switch (runch)
          {
            case 'h': if (x == hero.x + 1)
                        continue;
                      break;
            case 'j': if (y == hero.y - 1)
                        continue;
                      break;
            case 'k': if (y == hero.y + 1)
                        continue;
                      break;
            case 'l': if (x == hero.x - 1)
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
            case DOOR:    if (x == hero.x || y == hero.y)
                            running = false;
                          break;
            case PASSAGE: if (x == hero.x || y == hero.y)
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
    mvaddcch(hero.y, hero.x, PLAYER);
}

void
erase_lamp(coord *pos, struct room *rp)
{
  int y, x;

  if (!(see_floor && (rp->r_flags & (ISGONE|ISDARK)) == ISDARK
        && !is_blind(&player)))
    return;

  for (x = pos->x -1; x <= pos->x +1; x++)
    for (y = pos->y -1; y <= pos->y +1; y++)
    {
      if (y == hero.y && x == hero.x)
        continue;

      move(y, x);
      if (incch() == FLOOR)
        addcch(SHADOW);
    }
}

bool
show_floor(void)
{
  if ((proom->r_flags & (ISGONE|ISDARK)) == ISDARK && !is_blind(&player))
    return see_floor;
  else
    return true;
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
        pstats.s_exp++;
        msg("%s, this food tastes awful",
            is_hallucinating(&player) ? "bummer" : "yuk");
        check_level();
      }
      else
        msg("%s, that tasted good",
            is_hallucinating(&player) ? "oh, wow" : "yum");

    pack_remove(obj, false, false);
    return true;
}

void
check_level(void)
{
  int i, olevel;

  for (i = 0; e_levels[i] != 0; i++)
    if (e_levels[i] > pstats.s_exp)
      break;
  i++;
  olevel = pstats.s_lvl;
  pstats.s_lvl = i;
  if (i > olevel)
  {
    int add = roll(i - olevel, 10);
    max_hp += add;
    pstats.s_hpt += add;
    msg("welcome to level %d", i);
  }
}

void
chg_str(int amt)
{
  int i;
  str_t comp;

  if (amt == 0)
    return;

  add_str(&pstats.s_str, amt);
  comp = pstats.s_str;

  for (i = 0; i < RING_SLOTS_SIZE; ++i)
  {
    THING *ring = pack_equipped_item(ring_slots[i]);
    if (ring != NULL && ring->o_which == R_ADDSTR)
      add_str(&comp, -ring->o_arm);
  }

  if (comp > max_stats.s_str)
    max_stats.s_str = comp;
}

void
add_str(str_t *sp, int amt)
{
  if ((*sp += amt) < 3)
    *sp = 3;
  else if (*sp > 31)
    *sp = 31;
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

bool
get_dir(void)
{
  const char *prompt = terse ? "direction? " : "which direction? ";
  bool gotit;
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
          return false;

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
  if (is_confused(&player) && rnd(5) == 0)
    do
    {
      delta.y = rnd(3) - 1;
      delta.x = rnd(3) - 1;
    } while (delta.y == 0 && delta.x == 0);

  mpos = 0;
  return true;
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
call_it(struct obj_info *info)
{
  if (info->oi_know)
  {
    if (info->oi_guess)
    {
      free(info->oi_guess);
      info->oi_guess = NULL;
    }
  }
  else if (!info->oi_guess)
  {
    char tmpbuf[MAXSTR] = { '\0' };
    msg(terse ? "call it: " : "what do you want to call it? ");
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
  if (same_coords(hero, stairs)) /* It's under him */
    return true;

  /* if a monster is on the stairs, this gets hairy */
  if (tp != NULL)
  {
    if (see_monst(tp) && on(*tp, ISRUN)) /* if it's visible and awake */
      return true;                       /* it must have moved there */

    if (on(player, SEEMONST)             /* if she can detect monster */
        && tp->t_oldch == STAIRS)        /* and there once were stairs */
      return true;                       /* it must have moved there */
  }
  return false;
}

bool
turn_see(bool turn_off)
{
  THING *mp;
  bool add_new = false;

  for (mp = mlist; mp != NULL; mp = mp->l_next)
  {
    move(mp->t_pos.y, mp->t_pos.x);
    bool can_see = see_monst(mp);

    if (turn_off)
    {
      if (!can_see)
        addcch(mp->t_oldch);
    }
    else
    {
      if (is_hallucinating(&player))
        addcch((rnd(26) + 'A') | A_STANDOUT);
      else
        addcch(mp->t_type | A_STANDOUT);

      if (!can_see)
        add_new++;
    }
  }

  if (turn_off)
    player.t_flags &= ~SEEMONST;
  else
    player.t_flags |= SEEMONST;
  return add_new;
}

void
invis_on(void)
{
  THING *mp;

  player.t_flags |= CANSEE;
  for (mp = mlist; mp != NULL; mp = mp->l_next)
    if (is_invisible(mp) && see_monst(mp) && !is_hallucinating(&player))
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
    daemon_run_all(BEFORE);
    daemon_run_fuses(BEFORE);
    daemon_run_all(AFTER);
    daemon_run_fuses(AFTER);
  }
}

void
set_oldch(THING *tp, coord *cp)
{
  char sch = tp->t_oldch;

  if (same_coords(tp->t_pos, *cp))
    return;

  tp->t_oldch = mvincch(cp->y, cp->x);
  if (!is_blind(&player))
  {
    if ((sch == FLOOR || tp->t_oldch == FLOOR) &&
        (tp->t_room->r_flags & ISDARK))
      tp->t_oldch = SHADOW;
    else if (dist_cp(cp, &hero) <= LAMPDIST && see_floor)
      tp->t_oldch = chat(cp->y, cp->x);
  }
}

bool
see_monst(THING *mp)
{
  int y = mp->t_pos.y;
  int x = mp->t_pos.x;

  if (is_blind(&player) ||
      (is_invisible(mp) && !is_true_seeing(&player)))
    return false;

  if (dist(y, x, hero.y, hero.x) < LAMPDIST)
  {
    if (y != hero.y && x != hero.x &&
        !step_ok(chat(y, hero.x)) && !step_ok(chat(hero.y, x)))
      return false;
    return true;
  }

  if (mp->t_room != proom)
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
    struct room *rer;
    static coord tp;

    if (is_blind(&player))
	return false;
    if (dist(y, x, hero.y, hero.x) < LAMPDIST)
    {
	if (flat(y, x) & F_PASS)
	    if (y != hero.y && x != hero.x &&
		!step_ok(chat(y, hero.x)) && !step_ok(chat(hero.y, x)))
		    return false;
	return true;
    }
    /*
     * We can only see if the hero in the same room as
     * the coordinate and the room is lit or if it is close.
     */
    tp.y = y;
    tp.x = x;
    return (bool)((rer = roomin(&tp)) == proom && !(rer->r_flags & ISDARK));
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

    if (!see_monst(tp) && !on(player, SEEMONST))
	return (terse ? "it" : "something");
    else if (is_hallucinating(&player))
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
    return (is_hallucinating(&player) ? colors_random() : col);
}

int
player_save_throw(int which)
{
  if (which == VS_MAGIC)
  {
    int i;
    for (i = 0; i < RING_SLOTS_SIZE; ++i)
    {
      THING *ring = pack_equipped_item(ring_slots[i]);
      if (ring != NULL && ring->o_which == R_PROTECT)
        which -= ring->o_arm;
    }
  }
  return monster_save_throw(which, &player);
}

char
floor_ch(void)
{
  if (proom->r_flags & ISGONE)
    return PASSAGE;
  return show_floor() ? FLOOR : SHADOW;
}

char
floor_at(void)
{
  char ch = chat(hero.y, hero.x);
  return ch == FLOOR ? floor_ch() : ch;
}


void
reset_last(void)
{
  last_comm = l_last_comm;
  last_dir = l_last_dir;
  last_pick = l_last_pick;
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
