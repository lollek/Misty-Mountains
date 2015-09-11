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

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "io.h"
#include "item.h"
#include "level.h"
#include "list.h"
#include "misc.h"
#include "monster.h"
#include "options.h"
#include "os.h"
#include "pack.h"
#include "player.h"
#include "potions.h"
#include "rings.h"
#include "rogue.h"
#include "state.h"
#include "things.h"
#include "wand.h"
#include "weapons.h"

#include "scrolls.h"

static char* s_names[NSCROLLS];
struct obj_info scroll_info[NSCROLLS] = {
    { "monster confusion",		 7, 140, NULL, false },
    { "magic mapping",			 4, 150, NULL, false },
    { "hold monster",			 2, 180, NULL, false },
    { "sleep",				 3,   5, NULL, false },
    { "enchant armor",			 7, 160, NULL, false },
    { "identify",			43, 115, NULL, false },
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
scroll_init(void)
{
  char const* sylls[] = {
    "a", "ab", "ag", "aks", "ala", "an", "app", "arg", "arze", "ash",
    "bek", "bie", "bit", "bjor", "blu", "bot", "bu", "byt", "comp",
    "con", "cos", "cre", "dalf", "dan", "den", "do", "e", "eep", "el",
    "eng", "er", "ere", "erk", "esh", "evs", "fa", "fid", "fri", "fu",
    "gan", "gar", "glen", "gop", "gre", "ha", "hyd", "i", "ing", "ip",
    "ish", "it", "ite", "iv", "jo", "kho", "kli", "klis", "la", "lech",
    "mar", "me", "mi", "mic", "mik", "mon", "mung", "mur", "nej",
    "nelg", "nep", "ner", "nes", "nes", "nih", "nin", "o", "od", "ood",
    "org", "orn", "ox", "oxy", "pay", "ple", "plu", "po", "pot",
    "prok", "re", "rea", "rhov", "ri", "ro", "rog", "rok", "rol", "sa",
    "san", "sat", "sef", "seh", "shu", "ski", "sna", "sne", "snik",
    "sno", "so", "sol", "sri", "sta", "sun", "ta", "tab", "tem",
    "ther", "ti", "tox", "trol", "tue", "turs", "u", "ulk", "um", "un",
    "uni", "ur", "val", "viv", "vly", "vom", "wah", "wed", "werg",
    "wex", "whon", "wun", "xo", "y", "yot", "yu", "zant", "zeb", "zim",
    "zok", "zon", "zum",
  };

  int const MAXNAME = 40;
  for (int i = 0; i < NSCROLLS; i++)
  {
    char tmpbuf[MAXSTR*2];
    char* cp = tmpbuf;
    int nwords;

    for (nwords = os_rand_range(3)+2; nwords > 0; nwords--)
    {
      int nsyl = os_rand_range(3) + 1;

      while (nsyl--)
      {
        char const* sp = sylls[os_rand_range((sizeof(sylls)) / (sizeof(*sylls)))];
        if (&cp[strlen(sp)] > &tmpbuf[MAXNAME])
          break;
        while (*sp)
          *cp++ = *sp++;
      }

      *cp++ = ' ';
    }

    *--cp = '\0';
    s_names[i] = malloc((unsigned) strlen(tmpbuf)+1);
    strcpy(s_names[i], tmpbuf);
  }
}

bool
scroll_save_state(void)
{
  for (int i = 0; i < NSCROLLS; i++)
    if (state_save_string(s_names[i]))
      return 1;

  return state_save_obj_info(scroll_info, NSCROLLS);
}

bool
scroll_load_state(void)
{
  for (int i = 0; i < NSCROLLS; i++)
    if (state_load_string(&s_names[i]))
      return 1;

  return state_load_obj_info(scroll_info, NSCROLLS);
}

void scroll_learn(enum scroll_t scroll)
{
  scroll_info[scroll].oi_know = true;
}

bool
scroll_is_known(enum scroll_t scroll)
{
  return scroll_info[scroll].oi_know;
}

int
scroll_value(enum scroll_t scroll)
{
  return scroll_info[scroll].oi_worth;
}

void scroll_set_name(enum scroll_t scroll, char const* new_name)
{
  size_t len = strlen(new_name);

  if (scroll_info[scroll].oi_guess != NULL)
  {
    free(scroll_info[scroll].oi_guess);
    scroll_info[scroll].oi_guess = NULL;
  }

  if (len > 0)
  {
    scroll_info[scroll].oi_guess = malloc(len + 1);
    strcpy(scroll_info[scroll].oi_guess, new_name);
  }
}

static bool
enchant_players_armor(void)
{
  THING* arm = pack_equipped_item(EQUIPMENT_ARMOR);
  if (arm == NULL)
  {
    switch (os_rand_range(3))
    {
      case 0: io_msg("you are unsure if anything happened"); break;
      case 1: io_msg("you feel naked"); break;
      case 2: io_msg("you feel like something just touched you"); break;
    }
    return false;
  }

  arm->o.o_arm--;
  arm->o.o_flags &= ~ISCURSED;
  io_msg("your armor glows %s for a moment", pick_color("silver"));
  return true;
}

/* Stop all monsters within two spaces from chasing after the hero. */
static bool
hold_monsters(void)
{
  coord *player_pos = player_get_pos();
  int monsters_affected = 0;
  THING* held_monster = NULL;

  for (int x = player_pos->x - 2; x <= player_pos->x + 2; x++)
    if (x >= 0 && x < NUMCOLS)
      for (int y = player_pos->y - 2; y <= player_pos->y + 2; y++)
        if (y >= 0 && y <= NUMLINES - 1)
        {
          THING *monster = level_get_monster(y, x);
          if (monster != NULL)
          {
            monster_become_held(monster);
            monsters_affected++;
            held_monster = monster;
          }
        }

  if (monsters_affected == 1)
  {
    char buf[MAXSTR];
    io_msg("%s freezes", monster_name(held_monster, buf));
  }
  else if (monsters_affected > 1)
    io_msg("the monsters around you freeze");
  else /* monsters_affected == 0 */
    switch (os_rand_range(3))
    {
      case 0: io_msg("you are unsure if anything happened"); break;
      case 1: io_msg("you feel a strange sense of loss"); break;
      case 2: io_msg("you feel a powerful aura"); break;
    }

  return monsters_affected;
}

static bool
create_monster(void)
{
  const coord *player_pos = player_get_pos();
  coord mp;
  int i = 0;

  for (int y = player_pos->y - 1; y <= player_pos->y + 1; y++)
    for (int x = player_pos->x - 1; x <= player_pos->x + 1; x++)
    {
      char ch = level_get_type(y, x);

      if ((y == player_pos->y && x == player_pos->x)
          || !step_ok(ch)
          || (ch == SCROLL && find_obj(y, x)->o.o_which == S_SCARE)
          || os_rand_range(++i) != 0)
        continue;

      mp.y = y;
      mp.x = x;
    }

  if (i == 0)
    switch (os_rand_range(3))
    {
      case 0: io_msg("you are unsure if anything happened"); break;
      case 1: io_msg("you hear a faint cry of anguish in the distance"); break;
      case 2: io_msg("you think you felt someone's presence"); break;
    }
  else
  {
    char buf[MAXSTR];
    THING *obj = os_calloc_thing();
    monster_new(obj, monster_random(false), &mp);
    io_msg("A %s appears out of thin air", monster_name(obj, buf));
  }

  return i;
}

static void
magic_mapping(void)
{
  /* take all the things we want to keep hidden out of the window */
  for (int y = 1; y < NUMLINES - 1; y++)
    for (int x = 0; x < NUMCOLS; x++)
    {
      PLACE* pp = level_get_place(y, x);
      char ch = level_get_ch(y, x);
      switch (ch)
      {
        case DOOR: case STAIRS: break;

        case HWALL: case VWALL:
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
        THING* obj = pp->p_monst;
        if (obj != NULL)
          obj->t.t_oldch = ch;
        if (obj == NULL || !player_can_sense_monsters())
          mvaddcch(y, x, (chtype) ch);
      }
    }
}

static bool
food_detection(void)
{
  bool food_seen = false;
  wclear(hw);

  for (THING* obj = level_items; obj != NULL; obj = obj->o.l_next)
    if (obj->o.o_type == FOOD)
    {
      food_seen = true;
      mvwaddcch(hw, obj->o.o_pos.y, obj->o.o_pos.x, FOOD);
    }

  if (food_seen)
    show_win("Your nose tingles and you smell food.--More--");
  else
    io_msg("your nose tingles");

  return food_seen;
}

static bool
player_enchant_weapon(void)
{
  THING* weapon = pack_equipped_item(EQUIPMENT_RHAND);
  if (weapon == NULL)
  {
    switch (os_rand_range(2))
    {
      case 0: io_msg("you feel a strange sense of loss"); break;
      case 1: io_msg("you are unsure if anything happened"); break;
    }
    return false;
  }

  weapon->o.o_flags &= ~ISCURSED;
  if (os_rand_range(2) == 0)
    weapon->o.o_hplus++;
  else
    weapon->o.o_dplus++;
  io_msg("your %s glows %s for a moment",
      weapon_info[weapon->o.o_which].oi_name, pick_color("blue"));

  return true;
}

static void
remove_curse(void)
{
  for (int i = 0; i < NEQUIPMENT; ++i)
    if (pack_equipped_item((enum equipment_pos)i) != NULL)
      pack_uncurse_item(&pack_equipped_item((enum equipment_pos)i)->o);

  io_msg(player_is_hallucinating()
      ? "you feel in touch with the Universal Onenes"
      : "you feel as if somebody is watching over you");
}

static bool
protect_armor(void)
{
  THING* arm = pack_equipped_item(EQUIPMENT_ARMOR);
  if (arm == NULL)
  {
    switch (os_rand_range(2))
    {
      case 0: io_msg("you feel a strange sense of loss"); break;
      case 1: io_msg("you are unsure if anything happened"); break;
    }
    return false;
  }

  arm->o.o_flags |= ISPROT;
  io_msg("your armor is covered by a shimmering %s shield", pick_color("gold"));
  return true;
}

bool
scroll_read(void)
{
  THING* obj = pack_get_item("read", SCROLL);
  if (obj == NULL)
    return false;

  if (obj->o.o_type != SCROLL)
  {
    io_msg("there is nothing on it to read");
    return false;
  }

  /* Get rid of the thing */
  bool discardit = obj->o.o_count == 1;
  pack_remove(obj, false, false);
  THING* orig_obj = obj;

  switch (obj->o.o_which)
  {
    case S_CONFUSE:
      player_set_confusing_attack();
      break;
    case S_ARMOR:
      if (enchant_players_armor())
        scroll_learn(S_ARMOR);
      break;
    case S_HOLD:
      if (hold_monsters())
        scroll_learn(S_HOLD);
      break;
    case S_SLEEP:
      scroll_learn(S_SLEEP);
      player_fall_asleep();
      break;
    case S_CREATE:
      if (create_monster())
        scroll_learn(S_CREATE);
      break;
    case S_ID:
      if (!scroll_is_known(S_ID))
        io_msg("this scroll is an %s scroll", scroll_info[obj->o.o_which].oi_name);
      scroll_learn(S_ID);
      pack_identify_item();
      break;
    case S_MAP:
      scroll_learn(S_MAP);
      io_msg("this scroll has a map on it");
      magic_mapping();
      break;
    case S_FDET:
      if (food_detection())
        scroll_learn(S_FDET);
      break;
    case S_TELEP:
      scroll_learn(S_TELEP);
      player_teleport(NULL);
      break;
    case S_ENCH:
      player_enchant_weapon();
      break;
    case S_SCARE:
      /* Reading it is a mistake and produces laughter at her poor boo boo. */
      io_msg("you hear maniacal laughter in the distance");
      break;
    case S_REMOVE:
      remove_curse();
      break;
    case S_AGGR:
      /* This scroll aggravates all the monsters on the current
       * level and sets them running towards the hero */
      aggravate();
      io_msg("you hear a high pitched humming noise");
      break;
    case S_PROTECT:
      protect_armor();
      break;
    default:
      io_msg("what a puzzling scroll!");
      return true;
  }
  obj = orig_obj;
  look(true);	/* put the result of the scroll on the screen */
  io_refresh_statusline();

  call_it("scroll", &scroll_info[obj->o.o_which]);

  if (discardit)
    os_remove_thing(&obj);

  return true;
}

void
scroll_description(item const* item, char* buf)
{
  struct obj_info* op = &scroll_info[item_subtype(item)];
  char* ptr = buf;

  if (item_count(item) == 1)
    ptr += sprintf(ptr, "A scroll");
  else
    ptr += sprintf(ptr, "%d scrolls", item_count(item));

  if (op->oi_know)
    ptr += sprintf(ptr, " of %s", op->oi_name);
  else if (op->oi_guess)
    ptr += sprintf(ptr, " called %s", op->oi_guess);
  else
    ptr += sprintf(ptr, " titled '%s'", s_names[item_subtype(item)]);
}

THING*
scroll_create(int which)
{
  THING* scroll = os_calloc_thing();

  if (which == -1)
    which = (int)pick_one(scroll_info, NSCROLLS);

  scroll->o.o_type  = SCROLL;
  scroll->o.o_count = 1;
  scroll->o.o_which = which;
  return scroll;
}

