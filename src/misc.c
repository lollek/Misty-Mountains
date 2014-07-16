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

#include "rogue.h"
#include "status_effects.h"

static int trip_ch(int y, int x, int ch);

/** rnd:
 * Pick a very random number. */
int
rnd(int range)
{
  return range == 0 ? 0 : rand_r(&seed) % range;
}

/** roll:
 * Roll a number of dice
 */
int
roll(int number, int sides)
{
  int dtotal = 0;

  while (number--)
    dtotal += rnd(sides)+1;
  return dtotal;
}

/** look:
 * A quick glance all around the player */
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
        char *fp;
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
              wake_monster(y, x);
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
            when 'j': if (y == hero.y - 1)
                        continue;
            when 'k': if (y == hero.y + 1)
                        continue;
            when 'l': if (x == hero.x - 1)
                        continue;
            when 'y': if ((y + x) - sumhero >= 1)
                        continue;
            when 'u': if ((y - x) - diffhero >= 1)
                        continue;
            when 'n': if ((y + x) - sumhero <= -1)
                        continue;
            when 'b': if ((y - x) - diffhero <= -1)
                        continue;
          }
          switch (ch)
          {
            case DOOR:    if (x == hero.x || y == hero.y)
                            running = false;
            when PASSAGE: if (x == hero.x || y == hero.y)
                            passcount++;
            when FLOOR: case VWALL: case HWALL: case SHADOW:
              break;
            default: running = false;
          }
        }
      }
  if (door_stop && !firstmove && passcount > 1)
    running = false;
  if (!running || !jump)
    mvaddcch(hero.y, hero.x, PLAYER);
}

/** trip_ch:
 * Return the character appropriate for this space, taking into
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
        if (y != stairs.y || x != stairs.x || !seenstairs)
          return rnd_thing();
    }
  return ch;
}

/** erase_lamp:
 * Erase the area shown by a lamp in a dark room. */
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

/** show_floor:
 * Should we show the floor in her room at this time? */
bool
show_floor()
{
  if ((proom->r_flags & (ISGONE|ISDARK)) == ISDARK && !is_blind(&player))
    return see_floor;
  else
    return true;
}

/** find_obj:
 * Find the unclaimed object at y, x */
THING *
find_obj(int y, int x)
{
  THING *obj;

  for (obj = lvl_obj; obj != NULL; obj = next(obj))
    if (obj->o_pos.y == y && obj->o_pos.x == x)
      return obj;

  /* It should have returned by now */
  if (wizard)
    msg("Non-object %d,%d", y, x);
  return NULL;
}

/** eat:
 * She wants to eat something, so let her try */
void
eat()
{
    THING *obj = get_item("eat", FOOD);

    if (obj == NULL)
      return;

    if (obj->o_type != FOOD)
    {
      msg("that's inedible!");
      return;
    }

    food_left = (food_left > 0 ? food_left : 0) + HUNGERTIME - 200 + rnd(400);
    if (food_left > STOMACHSIZE)
      food_left = STOMACHSIZE;

    hungry_state = 0;

    if (obj == cur_weapon)
      cur_weapon = NULL;

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

    leave_pack(obj, false, false);
}

/** check_level:
 * Check to see if the guy has gone up a level */
void
check_level()
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

/** chg_str:
 * used to modify the playes strength.  It keeps track of the
 * highest it has been, just in case */
void
chg_str(int amt)
{
  str_t comp;

  if (amt == 0)
    return;

  add_str(&pstats.s_str, amt);
  comp = pstats.s_str;

  if (ISRING(LEFT, R_ADDSTR))
    add_str(&comp, -cur_ring[LEFT]->o_arm);

  if (ISRING(RIGHT, R_ADDSTR))
    add_str(&comp, -cur_ring[RIGHT]->o_arm);

  if (comp > max_stats.s_str)
    max_stats.s_str = comp;
}

/** add_str:
 * Perform the actual add, checking upper and lower bound limits */
void
add_str(str_t *sp, int amt)
{
  if ((*sp += amt) < 3)
    *sp = 3;
  else if (*sp > 31)
    *sp = 31;
}

/** aggravate:
 * Aggravate all the monsters on this level */
void
aggravate()
{
  THING *mp;
  for (mp = mlist; mp != NULL; mp = next(mp))
    runto(&mp->t_pos);
}

/** vowelstr:
 * For printfs: if string starts with a vowel, return "n" for an "an" */
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

/** is_in_use:
 * See if the object is one of the currently used items */
bool
is_in_use(THING *obj)
{
  if (obj == NULL)
    return false;

  if (obj == cur_armor || obj == cur_weapon || obj == cur_ring[LEFT]
      || obj == cur_ring[RIGHT])
  {
    msg(terse ? "in use" : "that's already in use");
    return true;
  }
  return false;
}

/** get_dir:
 * Set up the direction co_ordinate for use in varios "prefix" commands */
bool
get_dir()
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
        case 'h': case'H': delta.y =  0; delta.x = -1;
        when 'j': case'J': delta.y =  1; delta.x =  0;
        when 'k': case'K': delta.y = -1; delta.x =  0;
        when 'l': case'L': delta.y =  0; delta.x =  1;
        when 'y': case'Y': delta.y = -1; delta.x = -1;
        when 'u': case'U': delta.y = -1; delta.x =  1;
        when 'b': case'B': delta.y =  1; delta.x = -1;
        when 'n': case'N': delta.y =  1; delta.x =  1;
        when KEY_ESCAPE:
          last_dir = '\0';
          reset_last();
          msg("");
          return false;
        otherwise:
          mpos = 0;
          msg(prompt);
          gotit = false;
      }
    } until (gotit);

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

/** sign:
 * Return the sign of the number */
int
sign(int nm)
{
  if (nm < 0)
    return -1;
  else
    return (nm > 0);
}

/** spread:
 * Give a spread around a given number (+/- 20%) */
int
spread(int nm)
{
  return nm - nm / 20 + rnd(nm / 10);
}

/** call_it:
 * Call an object something after use */
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
    char tmpbuf[MAXSTR];
    msg(terse ? "call it: " : "what do you want to call it? ");
    if (get_str(tmpbuf, stdscr) == NORMAL)
    {
      if (info->oi_guess != NULL)
        free(info->oi_guess);
      info->oi_guess = malloc((unsigned int) strlen(tmpbuf) + 1);
      strcpy(info->oi_guess, tmpbuf);
    }
  }
}

/** rnd_thing:
 * Pick a random thing appropriate for this level */
char
rnd_thing()
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
             obj->o_arm != a_class[obj->o_which];

    case WEAPON:
      return obj->o_hplus != 0 || obj->o_dplus != 0;

    case POTION: case SCROLL: case STICK: case RING: case AMULET:
      return true;
  }
  return false;
}

bool
seen_stairs()
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

  for (mp = mlist; mp != NULL; mp = next(mp))
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
invis_on()
{
  THING *mp;

  player.t_flags |= CANSEE;
  for (mp = mlist; mp != NULL; mp = next(mp))
    if (is_invisible(mp) && see_monst(mp) && !is_hallucinating(&player))
      mvaddcch(mp->t_pos.y, mp->t_pos.x, mp->t_disguise);
}


