/*
 * Read a scroll and let it happen
 *
 * @(#)scrolls.c	4.44 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include "rogue.h"
#include "scrolls.h"
#include "status_effects.h"


struct obj_info scr_info[MAXSCROLLS] = {
    { "monster confusion",		 7, 140, NULL, false },
    { "magic mapping",			 4, 150, NULL, false },
    { "hold monster",			 2, 180, NULL, false },
    { "sleep",				 3,   5, NULL, false },
    { "enchant armor",			 7, 160, NULL, false },
    { "identify potion",		10,  80, NULL, false },
    { "identify scroll",		10,  80, NULL, false },
    { "identify weapon",		 6,  80, NULL, false },
    { "identify armor",		 	 7, 100, NULL, false },
    { "identify ring, wand or staff",	10, 115, NULL, false },
    { "scare monster",			 3, 200, NULL, false },
    { "food detection",			 2,  60, NULL, false },
    { "teleportation",			 5, 165, NULL, false },
    { "enchant weapon",			 8, 150, NULL, false },
    { "create monster",			 4,  75, NULL, false },
    { "remove curse",			 7, 105, NULL, false },
    { "aggravate monsters",		 3,  20, NULL, false },
    { "protect armor",			 2, 250, NULL, false },
};

void
read_scroll(void)
{
  THING *obj = get_item("read", SCROLL);
  THING *orig_obj;
  bool discardit = false;

  if (obj == NULL)
    return;

  if (obj->o_type != SCROLL)
  {
    msg(terse
      ? "nothing to read"
      : "there is nothing on it to read");
    return;
  }

  /* Calculate the effect it has on the poor guy. */
  if (obj == cur_weapon)
    cur_weapon = NULL;

  /* Get rid of the thing */
  discardit = (bool)(obj->o_count == 1);
  leave_pack(obj, false, false);
  orig_obj = obj;

  switch (obj->o_which)
  {
    case S_CONFUSE:
      /* Scroll of monster confusion.  Give him that power. */
      set_confusing(&player, true);
      msg("your hands begin to glow %s", pick_color("red"));
    when S_ARMOR:
      if (cur_armor != NULL)
      {
        cur_armor->o_arm--;
        cur_armor->o_flags &= ~ISCURSED;
        msg("your armor glows %s for a moment", pick_color("silver"));
      }
    when S_HOLD:
      /* Hold monster scroll.
       * Stop all monsters within two spaces from chasing after the hero. */
    {
      int x, y;
      char ch = 0;
      for (x = hero.x - 2; x <= hero.x + 2; x++)
        if (x >= 0 && x < NUMCOLS)
          for (y = hero.y - 2; y <= hero.y + 2; y++)
            if (y >= 0 && y <= NUMLINES - 1)
              if ((obj = moat(y, x)) != NULL && on(*obj, ISRUN))
              {
                obj->t_flags &= ~ISRUN;
                obj->t_flags |= ISHELD;
                ch++;
              }
      if (ch)
      {
        addmsg("the monster");
        if (ch > 1)
          addmsg("s around you");
        addmsg(" freeze");
        if (ch == 1)
          addmsg("s");
        endmsg();
        learn_scroll(S_HOLD);
      }
      else
        msg("you feel a strange sense of loss");
    }
    when S_SLEEP:
      learn_scroll(S_SLEEP);
      fall_asleep();
    when S_CREATE:
      /* Create a monster:
       * 1: look in a circle around him, 2: try his room, otherwise give up */
    {
      coord mp;
      int x, y;
      int i = 0;
      char ch;
      for (y = hero.y - 1; y <= hero.y + 1; y++)
        for (x = hero.x - 1; x <= hero.x + 1; x++)
          /* Don't put a monster in top of the player. */
          if (y == hero.y && x == hero.x)
            continue;
          /* Or anything else nasty */
          else if (step_ok(ch = winat(y, x)))
          {
            if (ch == SCROLL
                && find_obj(y, x)->o_which == S_SCARE)
              continue;
            else if (rnd(++i) == 0)
            {
              mp.y = y;
              mp.x = x;
            }
          }
      if (i == 0)
        msg("you hear a faint cry of anguish in the distance");
      else
      {
        obj = new_item();
        new_monster(obj, randmonster(false), &mp);
      }
    }
    when S_ID_POTION: case S_ID_SCROLL: case S_ID_WEAPON: case S_ID_ARMOR:
    case S_ID_R_OR_S:
    {
      static signed char id_type[S_ID_R_OR_S + 1] =
      { 0, 0, 0, 0, 0, POTION, SCROLL, WEAPON, ARMOR, R_OR_S };
      /* Identify, let him figure something out */
      learn_scroll(obj->o_which);
      msg("this scroll is an %s scroll", scr_info[obj->o_which].oi_name);
      whatis(id_type[obj->o_which]);
    }
    when S_MAP:
      /* Scroll of magic mapping. */
    {
      int x, y;
      char ch;
      learn_scroll(S_MAP);
      msg("oh, now this scroll has a map on it");
      /* take all the things we want to keep hidden out of the window */
      for (y = 1; y < NUMLINES - 1; y++)
        for (x = 0; x < NUMCOLS; x++)
        {
          PLACE *pp = INDEX(y, x);
          switch (ch = pp->p_ch)
          {
            case DOOR:
            case STAIRS:
              break;

            case HWALL:
            case VWALL:
              if (!(pp->p_flags & F_REAL))
              {
                ch = pp->p_ch = DOOR;
                pp->p_flags |= F_REAL;
              }
              break;

            case SHADOW:
              if (pp->p_flags & F_REAL)
                goto def;
              pp->p_flags |= F_REAL;
              ch = pp->p_ch = PASSAGE;
              /* FALLTHROUGH */

            case PASSAGE:
pass:
              if (!(pp->p_flags & F_REAL))
                pp->p_ch = PASSAGE;
              pp->p_flags |= (F_SEEN|F_REAL);
              ch = PASSAGE;
              break;

            case FLOOR:
              if (pp->p_flags & F_REAL)
                ch = SHADOW;
              else
              {
                ch = TRAP;
                pp->p_ch = TRAP;
                pp->p_flags |= (F_SEEN|F_REAL);
              }
              break;

            default:
def:
              if (pp->p_flags & F_PASS)
                goto pass;
              ch = SHADOW;
              break;
          }
          if (ch != SHADOW)
          {
            if ((obj = pp->p_monst) != NULL)
              obj->t_oldch = ch;
            if (obj == NULL || !on(player, SEEMONST))
              mvaddcch(y, x, ch);
          }
        }
    }
    when S_FDET:
        /* Potion of gold detection */
    {
      char ch = false;
      wclear(hw);
      for (obj = lvl_obj; obj != NULL; obj = next(obj))
        if (obj->o_type == FOOD)
        {
          ch = true;
          wmove(hw, obj->o_pos.y, obj->o_pos.x);
          waddcch(hw, FOOD);
        }
      if (ch)
      {
        learn_scroll(S_FDET);
        show_win("Your nose tingles and you smell food.--More--");
      }
      else
        msg("your nose tingles");
    }
    when S_TELEP:
      /* Scroll of teleportation: Make him dissapear and reappear */
    {
      struct room *cur_room = proom;
      teleport();
      if (cur_room != proom)
        learn_scroll(S_TELEP);
    }
    when S_ENCH:
      if (cur_weapon == NULL || cur_weapon->o_type != WEAPON)
        msg("you feel a strange sense of loss");
      else
      {
        cur_weapon->o_flags &= ~ISCURSED;
        if (rnd(2) == 0)
          cur_weapon->o_hplus++;
        else
          cur_weapon->o_dplus++;
        msg("your %s glows %s for a moment",
            weap_info[cur_weapon->o_which].oi_name, pick_color("blue"));
      }
    when S_SCARE:
      /* Reading it is a mistake and produces laughter at her poor boo boo. */
      msg("you hear maniacal laughter in the distance");
    when S_REMOVE:
      uncurse(cur_armor);
      uncurse(cur_weapon);
      uncurse(cur_ring[LEFT]);
      uncurse(cur_ring[RIGHT]);
      msg(is_hallucinating(&player)
          ? "you feel in touch with the Universal Onenes"
          : "you feel as if somebody is watching over you");
    when S_AGGR:
      /* This scroll aggravates all the monsters on the current
       * level and sets them running towards the hero */
      aggravate();
      msg("you hear a high pitched humming noise");
    when S_PROTECT:
      if (cur_armor != NULL)
      {
        cur_armor->o_flags |= ISPROT;
        msg("your armor is covered by a shimmering %s shield",
            pick_color("gold"));
      }
      else
        msg("you feel a strange sense of loss");
    otherwise:
      if (wizard)
      {
        msg("what a puzzling scroll!");
        return;
      }
  }
  obj = orig_obj;
  look(true);	/* put the result of the scroll on the screen */
  status();

  call_it(&scr_info[obj->o_which]);

  if (discardit)
    discard(obj);
}

void
uncurse(THING *obj)
{
    if (obj != NULL)
	obj->o_flags &= ~ISCURSED;
}

inline void
learn_scroll(enum scroll_t scroll)
{
  scr_info[scroll].oi_know = true;
}

inline bool
knows_scroll(enum scroll_t scroll)
{
  return scr_info[scroll].oi_know;
}
