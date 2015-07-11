#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "potions.h"
#include "scrolls.h"
#include "io.h"
#include "pack.h"
#include "daemons.h"
#include "colors.h"
#include "list.h"
#include "rings.h"
#include "save.h"
#include "passages.h"
#include "rooms.h"
#include "level.h"
#include "player.h"
#include "wand.h"
#include "monster.h"
#include "os.h"
#include "weapons.h"
#include "food.h"
#include "rogue.h"

#include "state.h"

#define SUCCESS 0

#define rs_assert(_a) if (_a) { io_msg("Error (#%d)", __LINE__); return 1; }

static FILE* file;

static bool
state_write(void const* buf, int32_t length)
{
  if (file == NULL)
    return fail("rs_write(%p, %d) File is NULL\r\n", buf, length);
  if (buf == NULL)
    return fail("rs_write(%p, %d) Buffer is NULL\r\n", buf, length);
  if (length <= 0)
    return fail("rs_write(%p, %d) Length is too small%d\r\n", buf, length);

  return io_encwrite(buf, (size_t)length, file) == (size_t)length
    ? SUCCESS
    : fail("rs_write(%p, %d)\r\n", buf, length);
}

bool state_save_int8(int8_t data)
{
  return state_write(&data, 1)
    ? fail("state_save_int8(%X)\r\n", data)
    : SUCCESS;
}

bool state_save_int32(int32_t data)
{
  return state_write(&data, 4)
    ? fail("state_save_int32(%X)\r\n", data)
    : SUCCESS;
}

static bool
state_save_bool(bool b)
{
  return state_save_int8(b)
    ? fail("state_save_bool(%b)\r\n", b)
    : SUCCESS;
}

bool
state_save_bools(bool const* c, int32_t count)
{
  if (state_save_int32(count))
    return fail("state_save_bools(%p, %d)\r\n", c, count);

  for (int32_t n = 0; n < count; n++)
    if (state_save_int8(c[n]))
      return fail("state_save_bools(%p, %d)\r\n", c, count);

  return SUCCESS;
}

static bool state_save_char(char c)
{
  return state_save_int8((int8_t)c)
    ? fail("state_save_char('%c')\r\n", c)
    : SUCCESS;
}

static bool
state_save_chars(char const* c, int32_t len)
{
  if (c == NULL)
    return state_save_int32(0)
      ? fail("state_save_chars(%p, %d)\r\n", c, len)
      : SUCCESS;

  return state_save_int32(len) || state_write(c, len)
    ? fail("state_save_chars(%p, %d)\r\n", c, len)
    : SUCCESS;
}

bool
state_save_string(char const* s)
{
  int32_t len;
  if (s == NULL)
    return state_save_int32(0) || state_save_chars(NULL, 0)
      ? fail("state_save_string(%p)\r\n", s)
      : SUCCESS;

  len = (int32_t) strlen(s) +1;
  return state_save_int32(len) || state_save_chars(s, len)
      ? fail("state_save_string(%p)\r\n", s)
      : SUCCESS;
}

bool
state_save_coord(coord const* c)
{
  return state_save_int32(c->y) || state_save_int32(c->x)
    ? fail("state_save_coord(%p)\r\n", c)
    : SUCCESS;
}

bool
state_save_struct_damage(struct damage const* dmg)
{
  return state_save_int32(dmg->sides) || state_save_int32(dmg->dices)
    ? fail("state_save_struct_damage(%p)\r\n", dmg)
    : SUCCESS;
}

bool
state_save_structs_damage(struct damage const dmg[MAXATTACKS])
{
  for (int i = 0; i < MAXATTACKS; ++i)
    if (state_save_struct_damage(&dmg[i]))
      return fail("state_save_structs_damage(%p)\r\n", dmg);
  return SUCCESS;
}

static bool
state_save_dest(coord const* dest)
{
  int32_t i;
  THING* ptr;

  /* (0,0): NULL - Not chasing anyone */
  if (dest == NULL)
    return state_save_int32(0) || state_save_int32(0)
      ? fail("state_save_dest(%p)\r\n", dest)
      : SUCCESS;

  /* (0,1): location of hero */
  if (dest == player_get_pos())
    return state_save_int32(0) || state_save_int32(1)
      ? fail("state_save_dest(%p)\r\n", dest)
      : SUCCESS;

  /* (1,i): location of a thing (monster) */
  for (ptr = monster_list, i = 0; ptr != NULL; ptr = ptr->t.l_next, ++i)
    if (&ptr->t.t_pos == dest)
      return state_save_int32(1) || state_save_int32(i)
        ? fail("state_save_dest(%p)\r\n", dest)
        : SUCCESS;

  /* (2,i): location of an object */
  for (ptr = level_items, i = 0; ptr != NULL; ptr = ptr->o.l_next, ++i)
    if (&ptr->o.o_pos == dest)
      return state_save_int32(2) || state_save_int32(i)
        ? fail("state_save_dest(%p)\r\n", dest)
        : SUCCESS;

  /* (3,i): location of gold in a room */
  for (i = 0; i < ROOMS_MAX; ++i)
    if (&rooms[i].r_gold == dest)
      return state_save_int32(3) || state_save_int32(i)
        ? fail("state_save_dest(%p)\r\n", dest)
        : SUCCESS;

  /* If we haven't found who we are chasing, we probably shouldnt chase anyone.
   * This was originally set to chase the hero by default */
  return state_save_int32(0) || state_save_int32(0)
    ? fail("state_save_dest(%p)\r\n", dest)
    : SUCCESS;
}

static bool
state_save_stats(struct stats const* s)
{
  return
    state_save_int32(RSID_STATS) ||
    state_save_int32(s->s_str) ||
    state_save_int32(s->s_exp) ||
    state_save_int32(s->s_lvl) ||
    state_save_int32(s->s_arm) ||
    state_save_int32(s->s_hpt) ||
    state_save_structs_damage(s->s_dmg) ||
    state_save_int32(s->s_maxhp)

    ? fail("state_save_stats(%p)\r\n", s)
    : SUCCESS;
}

static bool
state_save_room_number(struct room const* room)
{
  int i;
  for (i = 0; i < ROOMS_MAX; ++i)
    if (&rooms[i] == room)
      return state_save_int32(i)
        ? fail("state_save_room_number(%p)\r\n", room)
        : SUCCESS;
  return state_save_int32(-1)
    ? fail("state_save_room_number(%p)\r\n", room)
    : SUCCESS;
}

bool
state_save_thing(THING const* t)
{
  if (state_save_int32(RSID_THING))
    return fail("state_save_thing(%p)\r\n", t);

  if (t == NULL)
    return state_save_int32(0)
      ? fail("state_save_thing(%p)\r\n", t)
      : SUCCESS;

  return
    state_save_int32(1) ||
    state_save_coord(&t->t.t_pos) ||
    state_save_bool(t->t.t_turn) ||
    state_save_char(t->t.t_type) ||
    state_save_char(t->t.t_disguise) ||
    state_save_char(t->t.t_oldch) ||
    state_save_dest(t->t.t_dest) ||
    state_save_int32( t->t.t_flags) ||
    state_save_stats( &t->t.t_stats) ||
    state_save_room_number(t->t.t_room) ||
    state_save_list(t->t.t_pack)

    ? fail("state_save_thing(%p)\r\n", t)
    : SUCCESS;
}

static bool
state_save_object(THING const* o)
{
  return
    state_save_int32(RSID_OBJECT) ||
    state_save_int32(o->o.o_type) ||
    state_save_coord(&o->o.o_pos) ||
    state_save_int32(o->o.o_launch) ||
    state_save_char(o->o.o_packch) ||
    state_save_struct_damage(&o->o.o_damage) ||
    state_save_struct_damage(&o->o.o_hurldmg) ||
    state_save_int32(o->o.o_count) ||
    state_save_int32(o->o.o_which) ||
    state_save_int32(o->o.o_hplus) ||
    state_save_int32(o->o.o_dplus) ||
    state_save_int32(o->o.o_arm) ||
    state_save_int32(o->o.o_flags) ||
    state_save_string(o->o.o_label)

    ? fail("state_save_object(%p)\r\n", o)
    : SUCCESS;
}

static bool
state_save_equipment(enum equipment_pos slot, int32_t marker)
{
  THING* o = pack_equipped_item(slot);
  if (o == NULL)
    return state_save_int32(RSID_NULL)
      ? fail("state_save_object(%d, %d)\r\n", slot, marker)
      : SUCCESS;

  return state_save_int32(marker) || state_save_object(o)
    ? fail("state_save_object(%d, %d)\r\n", slot, marker)
    : SUCCESS;
}

static bool
state_save_list_index(THING const* list, THING const* item)
{
  if (item == NULL)
    return state_save_int32(-1)
      ? fail("state_save_list_index(%p, %p)\r\n", list, item)
      : SUCCESS;

  return state_save_int32(list_find(list, item))
    ? fail("state_save_list_index(%p, %p)\r\n", list, item)
    : SUCCESS;
}

static bool
state_save_places(void)
{
  int end = MAXLINES * MAXCOLS;
  int i;
  for (i = 0; i < end; ++i)
    if (state_save_char(level_places[i].p_ch) ||
        state_save_char(level_places[i].p_flags) ||
        state_save_list_index(monster_list, level_places[i].p_monst))
      return fail("state_save_places()\r\n", i);
  return SUCCESS;
}

static bool
state_save_room(struct room const* room)
{
  return
    state_save_coord(&room->r_pos) ||
    state_save_coord(&room->r_max) ||
    state_save_coord(&room->r_gold) ||
    state_save_int32(room->r_goldval) ||
    state_save_int32(room->r_flags) ||
    state_save_int32(room->r_nexits) ||
    state_save_coord(&room->r_exit[0]) ||
    state_save_coord(&room->r_exit[1]) ||
    state_save_coord(&room->r_exit[2]) ||
    state_save_coord(&room->r_exit[3]) ||
    state_save_coord(&room->r_exit[4]) ||
    state_save_coord(&room->r_exit[5]) ||
    state_save_coord(&room->r_exit[6]) ||
    state_save_coord(&room->r_exit[7]) ||
    state_save_coord(&room->r_exit[8]) ||
    state_save_coord(&room->r_exit[9]) ||
    state_save_coord(&room->r_exit[10]) ||
    state_save_coord(&room->r_exit[11])

    ? fail("state_save_room(%p)\r\n", room)
    : SUCCESS;
}

static bool
state_save_rooms(struct room const list[], int32_t number)
{
  int i;
  if (state_save_int32(number))
    return fail("state_save_rooms(%p, %d)\r\n", list, number);
  for (i = 0; i < number; ++i)
    if (state_save_room(&list[i]))
      return fail("state_save_rooms(%p, %d)\r\n", list, number);
  return SUCCESS;
}

static bool
state_save_daemons(struct delayed_action const* d_list, int32_t count)
{
  int i = 0;

  if (state_save_int32(RSID_DAEMONS) ||
      state_save_int32(count))
    return fail("state_save_daemons(%p, %d)\r\n", d_list, count);

  for (i = 0; i < count; i++)
  {
    int func;
    if (d_list[i].d_func == NULL)                              func = 0;
    else if (d_list[i].d_func == daemon_rollwand)              func = 1;
    else if (d_list[i].d_func == daemon_doctor)                func = 2;
    else if (d_list[i].d_func == daemon_runners_move)          func = 3;
    else if (d_list[i].d_func == daemon_start_wanderer)        func = 4;
    else if (d_list[i].d_func == player_decrease_speed)        func = 5;
    else if (d_list[i].d_func == player_remove_confused)       func = 6;
    else if (d_list[i].d_func == player_remove_true_sight)     func = 7;
    else if (d_list[i].d_func == player_remove_blind)          func = 8;
    else if (d_list[i].d_func == daemon_ring_abilities)        func = 9;
    else if (d_list[i].d_func == player_remove_sense_monsters) func = 10;
    else if (d_list[i].d_func == player_remove_hallucinating)  func = 11;
    else if (d_list[i].d_func == player_stop_levitating)       func = 12;
    else if (d_list[i].d_func == daemon_change_visuals)        func = 13;
    else
      return fail("state_save_daemons(%p, %d) Unknown Daemon\r\n",
                     d_list, count);

    if (state_save_int32(d_list[i].d_type) ||
        state_save_int32(func) ||
        state_save_int32(d_list[i].d_arg) ||
        state_save_int32(d_list[i].d_time))
      return fail("state_save_daemons(%p, %d)\r\n", d_list, count);
  }
  return 0;
}

static bool
state_save_window(WINDOW* win)
{
  int row;
  int col;
  int32_t height = getmaxy(win);
  int32_t width  = getmaxx(win);

  if (state_save_int32(RSID_WINDOW) ||
      state_save_int32(height) ||
      state_save_int32(width))
    return fail("state_save_window(%p)\r\n", win);

  for (row = 0; row < height; row++)
    for (col = 0; col < width; col++)
    {
      int32_t sym = mvwincch(win, row, col);
      if (state_save_int32(sym))
        return fail("state_save_window(%p)\r\n", win);
    }

  return 0;

}

bool
state_save_obj_info(const struct obj_info* i, int count)
{
  if (state_save_int32(RSID_MAGICITEMS) ||
      state_save_int32(count))
    return fail("state_save_obj_info(%p, %d)\r\n", i, count);

  for (int n = 0; n < count; n++)
    if(state_save_int32(i[n].oi_prob) ||
       state_save_int32(i[n].oi_worth) ||
       state_save_string(i[n].oi_guess) ||
       state_save_int8(i[n].oi_know))
      return fail("state_save_obj_info(%p, %d)\r\n", i, count);
  return 0;
}

bool
state_save_list(const THING* l)
{
  int32_t listsize;
  THING const* ptr;

  for (ptr = l, listsize = 0; ptr != NULL; ptr = ptr->o.l_next)
    ++listsize;

  if (state_save_int32(RSID_OBJECTLIST) ||
      state_save_int32(listsize))
    return fail("state_save_list(%p)\r\n", l);

  for(; l != NULL; l = l->o.l_next)
    if (state_save_object(l))
      return fail("state_save_list(%p)\r\n", l);

  return 0;
}

bool
state_save_index(char const** master, int max, char const* str)
{
  int32_t i;
  for(i = 0; i < max; i++)
    if (str == master[i])
      break;

  return state_save_int32(i == max ? -1 : i)
    ?  fail("state_save_index(%p[%p], %d, %p)\r\n", master, *master, max, str)
    : SUCCESS;
}

bool
state_save_file(FILE* savef)
{
  int32_t maxstr = MAXSTR;
  char buf[80];

  assert(savef != NULL);
  assert(file == NULL);
  file = savef;

  rs_assert(state_write(GAME_VERSION, sizeof(GAME_VERSION)));
  sprintf(buf, "%d x %d\n", LINES, COLS);
  rs_assert(state_write(buf, sizeof(buf)));

  state_save_int32(RSID_START);

  rs_assert(state_save_int32(RSID_PACK)    ||   pack_save_state());
  rs_assert(state_save_int32(RSID_POTIONS) || potion_save_state());
  rs_assert(state_save_int32(RSID_RINGS)   ||   ring_save_state());
  rs_assert(state_save_int32(RSID_SCROLLS) || scroll_save_state());
  rs_assert(state_save_int32(RSID_WANDS)   ||   wand_save_state());
  rs_assert(state_save_int32(RSID_PLAYER)  || player_save_state());
  rs_assert(state_save_int32(RSID_LEVEL)   ||  level_save_state());
  rs_assert(state_save_int32(RSID_FOOD)    ||   food_save_state());

  rs_assert(state_save_bool(firstmove))
  rs_assert(state_save_chars(save_file_name, maxstr))
  rs_assert(state_save_char(runch))
  rs_assert(state_save_chars(whoami, maxstr))
  rs_assert(state_save_int32(levels_without_food))
  rs_assert(state_save_int32(player_turns_without_moving))
  rs_assert(state_save_int32(pack_gold))
  rs_assert(state_save_int32(monster_flytrap_hit))
  rs_assert(state_save_int32((int32_t)os_rand_seed))

  rs_assert(state_save_equipment(EQUIPMENT_ARMOR, RSID_ARMOR))
  rs_assert(state_save_equipment(EQUIPMENT_RHAND, RSID_RHAND))
  rs_assert(state_save_equipment(EQUIPMENT_RRING, RSID_RRING))
  rs_assert(state_save_equipment(EQUIPMENT_LRING, RSID_LRING))

  rs_assert(monsters_save_state());
  rs_assert(weapons_save_state());
  rs_assert(state_save_places());
  rs_assert(state_save_stats(&player_max_stats))
  rs_assert(state_save_rooms( rooms, ROOMS_MAX))
  rs_assert(state_save_room_number(room_prev))
  rs_assert(state_save_rooms(passages, PASSAGES_MAX))
  rs_assert(state_save_obj_info(potion_info,  NPOTIONS))
  rs_assert(state_save_obj_info(ring_info,  NRINGS))
  rs_assert(state_save_daemons(__daemons_ptr(), 20))
  rs_assert(state_save_window(stdscr))

  state_save_int32(RSID_END);

  file = NULL;
  assert(file == NULL);

  return SUCCESS;
}

