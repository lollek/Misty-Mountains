#include <assert.h>

#include <string>
#include <vector>

using namespace std;

#include "Coordinate.h"
#include "fight.h"
#include "io.h"
#include "item.h"
#include "level.h"
#include "magic.h"
#include "misc.h"
#include "monster.h"
#include "options.h"
#include "os.h"
#include "pack.h"
#include "passages.h"
#include "player.h"
#include "rogue.h"
#include "things.h"
#include "weapons.h"

#include "wand.h"

static vector<string> material = {
  /* Wood */
  "avocado wood", "balsa", "bamboo", "banyan", "birch", "cedar", "cherry",
  "cinnibar", "cypress", "dogwood", "driftwood", "ebony", "elm", "eucalyptus",
  "fall", "hemlock", "holly", "ironwood", "kukui wood", "mahogany",
  "manzanita", "maple", "oaken", "persimmon wood", "pecan", "pine", "poplar",
  "redwood", "rosewood", "spruce", "teak", "walnut", "zebrawood",

  /* Metal */
  "aluminum", "beryllium", "bone", "brass", "bronze", "copper", "electrum",
  "gold", "iron", "lead", "magnesium", "mercury", "nickel", "pewter",
  "platinum", "steel", "silver", "silicon", "tin", "titanium", "tungsten",
  "zinc",
};

static vector<string*> _wand_material(MAXSTICKS);

vector<obj_info> wands_info = {
    { "light",			12, 250,   "", false },
    { "invisibility",		 6,   5,   "", false },
    { "lightning",		 3, 330,   "", false },
    { "fire",			 3, 330,   "", false },
    { "cold",			 3, 330,   "", false },
    { "polymorph",		15, 310,   "", false },
    { "magic missile",		10, 170,   "", false },
    { "haste monster",		10,   5,   "", false },
    { "slow monster",		11, 350,   "", false },
    { "drain life",		 9, 300,   "", false },
    { "nothing",		 1,   5,   "", false },
    { "teleport away",		 6, 340,   "", false },
    { "teleport to",		 6,  50,   "", false },
    { "cancellation",		 5, 280,   "", false },
};

void wand_init(void)
{
  vector<bool> used(material.size());

  for (size_t i = 0; i < material.size(); i++)
    used.at(i) = false;

  for (size_t i = 0; i < MAXSTICKS; i++)
  {
    size_t j = os_rand_range(material.size());

    while (used.at(j)) {
      j = os_rand_range(material.size());
    }

    _wand_material.at(i) = &material.at(j);
    used.at(j) = true;
  }
}

string const&
wand_material(enum wand_t wand)
{
  return *_wand_material.at(wand);
}

Item*
wand_create(int wand)
{
  Item* new_wand = new Item();

  new_wand->o_damage  = {1, 1};
  new_wand->o_hurldmg = {1, 1};

  new_wand->o_arm = 11;
  new_wand->o_count = 1;

  new_wand->o_type = STICK;
  if (wand < 0 || wand >= MAXSTICKS)
    new_wand->o_which = static_cast<int>(pick_one(wands_info, MAXSTICKS));
  else
    new_wand->o_which = wand;

  switch (new_wand->o_which)
  {
    case WS_LIGHT: new_wand->o_charges = os_rand_range(10) + 10; break;
    default:       new_wand->o_charges = os_rand_range(5) + 3;   break;
  }

  return new_wand;
}

/* This can only be used inside the wand_zap() function */
static Monster*
wand_find_target(int* y, int* x, int dy, int dx)
{
  *y = player_y();
  *x = player_x();

  /* "walk" in the zap direction until we find a target */
  while (step_ok(level_get_type(*y, *x)))
  {
    *y += dy;
    *x += dx;
  }

  return level_get_monster(*y, *x);
}

static void
wand_spell_light(void)
{
  if (player_get_room()->r_flags & ISGONE)
  {
    io_msg("the corridor glows and then fades");
    return;
  }

  player_get_room()->r_flags &= ~ISDARK;
  room_enter(player_get_pos());
  io_msg("the rooms is lit by a shimmering %s light", pick_color("blue"));
}

/* take away 1/2 of hero's hit points, then take it away
 * evenly from the monsters in the room (or next to hero
 * if he is in a passage) */
static void
wand_spell_drain_health(void)
{
  Monster* drainee[40];
  Coordinate* player_pos = player_get_pos();

  /* First cnt how many things we need to spread the hit points among */
  struct room *corp = level_get_ch(player_pos->y, player_pos->x) == DOOR
    ? &passages[level_get_flags(player_pos->y, player_pos->x) & F_PNUM]
    : nullptr;
  Monster** dp = drainee;

  int cnt = monster_add_nearby(dp, corp);
  if (cnt == 0)
  {
    io_msg("you have a tingling feeling");
    return;
  }

  *dp = nullptr;
  player_lose_health(player_get_health() / 2);
  io_msg("You feel an intense pain");
  cnt = player_get_health() / cnt;

  /* Now zot all of the monsters */
  for (dp = drainee; *dp; dp++)
  {
    Monster* mp = *dp;
    mp->t_stats.s_hpt -= cnt;
    if (mp->t_stats.s_hpt <= 0)
      monster_on_death(mp, monster_seen_by_player(mp));
    else
    {
      monster_start_running(&mp->t_pos);
      char buf[MAXSTR];
      io_msg("%s screams in pain", monster_name(mp, buf));
    }
  }
}

static void
wand_spell_polymorph(Monster* target)
{
  assert(target != nullptr);
  monster_polymorph(target);
  wands_info.at(WS_POLYMORPH).oi_know |= monster_seen_by_player(target);
}

static void
wand_spell_cancel(Monster* target)
{
  assert(target != nullptr);

  if (target->t_type == 'F')
    player_remove_held();

  monster_set_cancelled(target);
  monster_remove_invisible(target);
  monster_remove_confusing(target);

  target->t_disguise = target->t_type;
  if (monster_seen_by_player(target))
    mvaddcch(target->t_pos.y, target->t_pos.x, static_cast<chtype>(target->t_disguise));
}

static void
wand_spell_magic_missile(int dy, int dx)
{
  Item bolt;
  memset(&bolt, 0, sizeof(bolt));
  bolt.o_type    = '*';
  bolt.o_hplus   = 100;
  bolt.o_dplus   = 1;
  bolt.o_flags   = ISMISL;
  bolt.o_damage  = {0, 0};
  bolt.o_hurldmg = {1, 4};

  Item* weapon = pack_equipped_item(EQUIPMENT_RHAND);
  if (weapon != nullptr)
    bolt.o_launch = weapon->o_which;

  io_missile_motion(&bolt, dy, dx);

  Monster* target = level_get_monster(bolt.get_y(), bolt.get_x());
  if (target == nullptr)
    io_msg("the missle vanishes with a puff of smoke");
  else if (monster_save_throw(VS_MAGIC, target))
  {
    char buf[MAXSTR];
    io_msg("the missle missed the %s", monster_name(target, buf));
  }
  else
    fight_against_monster(&bolt.get_pos(), &bolt, true);
}



bool
wand_zap(void)
{
  Coordinate const* dir = get_dir();
  if (dir == nullptr)
    return false;
  Coordinate delta = *dir;

  Item* obj = pack_get_item("zap with", STICK);
  if (obj == nullptr)
    return false;
  else if (obj->o_type != STICK)
  {
    io_msg("you can't zap with that!");
    return false;
  }
  else if (obj->o_charges == 0)
  {
    io_msg("nothing happens");
    return true;
  }

  assert(obj->o_which >= 0 && obj->o_which < MAXSTICKS);

  switch (obj->o_which)
  {
    case WS_LIGHT:
      wands_info.at(WS_LIGHT).oi_know = true;
      wand_spell_light();
      break;

    case WS_DRAIN:
      if (player_get_health() > 1)
        wand_spell_drain_health();
      else
      {
        io_msg("you are too weak to use it");
        return true;
      }
      break;

    case WS_INVIS:
      {
        Coordinate c;
        Monster* tp = wand_find_target(&c.y, &c.x, delta.y, delta.x);
        if (tp != nullptr)
          monster_set_invisible(tp);
        else
          io_msg("You did not hit anything");
      } break;

    case WS_POLYMORPH:
      {
        Coordinate c;
        Monster* tp = wand_find_target(&c.y, &c.x, delta.y, delta.x);
        if (tp != nullptr)
          wand_spell_polymorph(tp);
        else
          io_msg("You did not hit anything");
      } break;

    case WS_CANCEL:
      {
        Coordinate c;
        Monster* tp = wand_find_target(&c.y, &c.x, delta.y, delta.x);
        if (tp != nullptr)
          wand_spell_cancel(tp);
        else
          io_msg("You did not hit anything");
      } break;

    case WS_TELAWAY:
      wands_info.at(WS_TELAWAY).oi_know = true;
      player_teleport(nullptr);
      break;

    case WS_TELTO:
      {
        wands_info.at(WS_TELTO).oi_know = true;
        int x;
        int y;
        Monster* tp = wand_find_target(&y, &x, delta.y, delta.x);
        if (tp != nullptr)
        {
          Coordinate new_pos;
          new_pos.y = y - delta.y;
          new_pos.x = x - delta.x;

          tp->t_dest = *player_get_pos();
          tp->t_flags |= ISRUN;

          player_teleport(&new_pos);
        }
        else
          io_msg("You did not hit anything");
      } break;

    case WS_MISSILE:
      wands_info.at(WS_MISSILE).oi_know = true;
      wand_spell_magic_missile(delta.y, delta.x);
      break;

    case WS_HASTE_M:
      {
        Coordinate c;
        Monster* tp = wand_find_target(&c.y, &c.x, delta.y, delta.x);
        if (tp != nullptr)
        {
          if (monster_is_slow(tp))
            tp->t_flags &= ~ISSLOW;
          else
            tp->t_flags |= ISHASTE;
          monster_start_running(&c);
          char buf[MAXSTR];
          io_msg("%s became faster", monster_name(tp, buf));
        }
        else
          io_msg("You did not hit anything");
      }
      break;

    case WS_SLOW_M:
      {
        Coordinate c;
        Monster* tp = wand_find_target(&c.y, &c.x, delta.y, delta.x);
        if (tp != nullptr)
        {
          if (monster_is_hasted(tp))
            tp->t_flags &= ~ISHASTE;
          else
            tp->t_flags |= ISSLOW;
          tp->t_turn = true;
          monster_start_running(&c);
          char buf[MAXSTR];
          io_msg("%s became slower", monster_name(tp, buf));
        }
        else
          io_msg("You did not hit anything");
      }
      break;

    case WS_ELECT:
      wands_info.at(WS_ELECT).oi_know = true;
      magic_bolt(player_get_pos(), &delta, "bolt");
      break;

    case WS_FIRE:
      wands_info.at(WS_FIRE).oi_know = true;
      magic_bolt(player_get_pos(), &delta, "flame");
      break;

    case WS_COLD:
      wands_info.at(WS_COLD).oi_know = true;
      magic_bolt(player_get_pos(), &delta, "ice");
      break;

    case WS_NOP:
      io_msg("You are usure if anything happened");
      break;

    default:
      io_msg("what a bizarre schtick!");
      break;
    }

    obj->o_charges--;
    return true;
}

char*
wand_description(Item const* item, char* buf)
{
  char* ptr = buf;
  struct obj_info oi = wands_info.at(static_cast<size_t>(item_subtype(item)));

  if (oi.oi_know || !oi.oi_guess.empty())
  {
    if (item_count(item))
      strcpy(ptr, "A wand");
    else
      sprintf(ptr, "%d wands", item_count(item));

    ptr += strlen(ptr);
    if (oi.oi_know)
      sprintf(ptr, " of %s", oi.oi_name.c_str());
    else if (!oi.oi_guess.empty())
      sprintf(ptr, " called %s", oi.oi_guess.c_str());

    ptr += strlen(ptr);
    if (item_is_known(item))
      sprintf(ptr, " [%d charges]", item_charges(item));

    ptr += strlen(ptr);
    sprintf(ptr, " (%s)", wand_material(static_cast<wand_t>(item_subtype(item))).c_str());
  }
  else if (item_count(item) == 1)
    sprintf(ptr, "A %s wand", wand_material(static_cast<wand_t>(item_subtype(item))).c_str());
  else
    sprintf(ptr, "%d %s wands", item_count(item),
            wand_material(static_cast<wand_t>(item_subtype(item))).c_str());

  return buf;
}

bool wand_is_known(enum wand_t wand)
{
  return wands_info.at(wand).oi_know;
}
void wand_set_known(enum wand_t wand)
{
  wands_info.at(wand).oi_know = true;
}

void wand_set_name(enum wand_t wand, string const& new_name)
{
  wands_info.at(wand).oi_guess = new_name;
}

size_t wand_get_worth(enum wand_t wand)
{
  return wands_info.at(wand).oi_worth;
}
