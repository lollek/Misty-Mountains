/*
    state.c - Portable Rogue Save State Code

    Copyright (C) 1999, 2000, 2005 Nicholas J. Kisseberth
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name(s) of the author(s) nor the names of other contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/

#include <string.h>

#include "rogue.h"
#include "potions.h"
#include "status_effects.h"
#include "scrolls.h"
#include "io.h"
#include "chase.h"
#include "pack.h"

static const size_t RSID_STATS        = 0xABCD0001;
static const size_t RSID_THING        = 0xABCD0002;
static const size_t RSID_OBJECT       = 0xABCD0003;
static const size_t RSID_MAGICITEMS   = 0xABCD0004;
static const size_t RSID_OBJECTLIST   = 0xABCD0007;
static const size_t RSID_MONSTERLIST  = 0xABCD0009;
static const size_t RSID_WINDOW       = 0xABCD000D;
static const size_t RSID_DAEMONS      = 0xABCD000E;

static const size_t RSID_ARMOR        = 0xABCD0010;
static const size_t RSID_RHAND        = 0xABCD0011;
static const size_t RSID_RRING        = 0xABCD0013;
static const size_t RSID_LRING        = 0xABCD0012;
static const size_t RSID_NULL         = 0xABCD0000;

#define rs_assert(_a) if (_a) { return printf("L%d@%s", __LINE__, __FILE__); }


#define rs_write(_f, _p, _s)     (encwrite(_p, _s, _f) != _s)
#define rs_write_boolean(_f, _c) rs_write(_f, (char *)&_c, 1)
#define rs_write_char(_f, _c)    rs_write(_f, (char *)&_c, 1)
#define rs_write_short(_f, _c)   rs_write(_f, (char *)&_c, 2)
#define rs_write_int(_f, _c)     rs_write(_f, (char *)&_c, 4)
#define rs_write_marker          rs_write_int
#define rs_write_str_t           rs_write_int
#define rs_write_coord(_f, _c) \
  (rs_write_int(_f, (_c).x) || \
   rs_write_int(_f, (_c).y))
#define rs_write_chars(_f, _c, _n) \
  (rs_write_int(_f, _n) || \
   rs_write(_f, _c, _n))
#define rs_write_room(_f, _r) \
  (rs_write_coord(savef, (_r)->r_pos) || \
   rs_write_coord(savef, (_r)->r_max) || \
   rs_write_coord(savef, (_r)->r_gold) || \
   rs_write_int(savef,   (_r)->r_goldval) || \
   rs_write_short(savef, (_r)->r_flags) || \
   rs_write_int(savef,   (_r)->r_nexits) || \
   rs_write_coord(savef, (_r)->r_exit[0]) || \
   rs_write_coord(savef, (_r)->r_exit[1]) || \
   rs_write_coord(savef, (_r)->r_exit[2]) || \
   rs_write_coord(savef, (_r)->r_exit[3]) || \
   rs_write_coord(savef, (_r)->r_exit[4]) || \
   rs_write_coord(savef, (_r)->r_exit[5]) || \
   rs_write_coord(savef, (_r)->r_exit[6]) || \
   rs_write_coord(savef, (_r)->r_exit[7]) || \
   rs_write_coord(savef, (_r)->r_exit[8]) || \
   rs_write_coord(savef, (_r)->r_exit[9]) || \
   rs_write_coord(savef, (_r)->r_exit[10]) || \
   rs_write_coord(savef, (_r)->r_exit[11]))
#define rs_assert_write_places(_f, _p, _n) \
  for (temp_i = 0; temp_i < _n; ++temp_i) \
    rs_assert(rs_write_char(_f, (_p)[temp_i].p_ch) || \
              rs_write_char(_f, (_p)[temp_i].p_flags) || \
              rs_write_thing_reference(_f, mlist, (_p)[temp_i].p_monst))

#define rs_read(_f, _p, _s)     (encread(_p, _s, _f) != _s)
#define rs_read_boolean(_f, _i) rs_read(_f, (char *)_i, 1)
#define rs_read_char(_f, _c)    rs_read(_f, (char *)_c, 1)
#define rs_read_short(_f, _i)   rs_read(_f, (char *)_i, 2)
#define rs_read_int(_f, _i)     rs_read(_f, (char *)_i, 4)
#define rs_read_str_t           rs_read_int
#define rs_read_coord(_f, _c) \
  (rs_read_int(_f, &(_c)->x) || \
   rs_read_int(_f, &(_c)->y))
#define rs_read_chars(_f, _i, _n) \
  (rs_read_int(_f, &temp_i) || \
   temp_i != _n || rs_read(_f, _i, _n))
#define rs_read_marker(_f, _i) \
  (rs_read_int(_f, &temp_i) || \
   _i != temp_i)
#define rs_read_stats(_f, _s) \
  (rs_read_marker(_f, RSID_STATS) || \
   rs_read_str_t(_f, &(_s)->s_str) || \
   rs_read_int(_f, &(_s)->s_exp) || \
   rs_read_int(_f, &(_s)->s_lvl) || \
   rs_read_int(_f, &(_s)->s_arm) || \
   rs_read_int(_f, &(_s)->s_hpt) || \
   rs_read_chars(_f, (_s)->s_dmg, sizeof((_s)->s_dmg)) || \
   rs_read_int(_f, &(_s)->s_maxhp))
#define rs_read_room(_f, _r) \
  (rs_read_coord(inf,&(_r)->r_pos) || \
   rs_read_coord(inf,&(_r)->r_max) || \
   rs_read_coord(inf,&(_r)->r_gold) || \
   rs_read_int(inf,&(_r)->r_goldval) || \
   rs_read_short(inf,&(_r)->r_flags) || \
   rs_read_int(inf,&(_r)->r_nexits) || \
   rs_read_coord(inf,&(_r)->r_exit[0]) || \
   rs_read_coord(inf,&(_r)->r_exit[1]) || \
   rs_read_coord(inf,&(_r)->r_exit[2]) || \
   rs_read_coord(inf,&(_r)->r_exit[3]) || \
   rs_read_coord(inf,&(_r)->r_exit[4]) || \
   rs_read_coord(inf,&(_r)->r_exit[5]) || \
   rs_read_coord(inf,&(_r)->r_exit[6]) || \
   rs_read_coord(inf,&(_r)->r_exit[7]) || \
   rs_read_coord(inf,&(_r)->r_exit[8]) || \
   rs_read_coord(inf,&(_r)->r_exit[9]) || \
   rs_read_coord(inf,&(_r)->r_exit[10]) || \
   rs_read_coord(inf,&(_r)->r_exit[11]))
#define rs_assert_read_places(_f, _p, _n) \
  for (temp_i = 0; temp_i < _n; ++temp_i) \
    rs_assert(rs_read_char(_f,&(_p)[temp_i].p_ch) || \
              rs_read_char(_f,&(_p)[temp_i].p_flags) || \
              rs_read_thing_reference(_f, mlist, &(_p)[temp_i].p_monst))


static int
rs_write_booleans(FILE *savef, const bool *c, int count)
{
  int n;

  if (rs_write_int(savef, count))
    return 1;

  for(n = 0; n < count; n++)
    if (rs_write_boolean(savef, c[n]) != 0)
      return 1;
  return 0;
}

static int
rs_read_booleans(FILE *inf, bool *i, int count)
{
  int n = 0;

  if (rs_read_int(inf,&n) ||
      n != count)
    return 1;

  for(n = 0; n < count; n++)
    if (rs_read_boolean(inf, &i[n]))
      return 1;;
  return 0;
}

static int
rs_write_string(FILE *savef, const char *s)
{
  size_t len = (s == NULL) ? 0 : (int) strlen(s) + 1;
  return rs_write_int(savef, len) ||
    rs_write_chars(savef, s, len);
}

static int
rs_read_new_string(FILE *inf, char **s)
{
  size_t temp_i = 0; /* Used by rs_read_chars macro */
  size_t len = 0;
  char *buf = 0;

  if (rs_read_int(inf, &len))
    return 1;

  if (len == 0)
    buf = NULL;
  else if ((buf = malloc(len)) == NULL)
    return 1;

  if (rs_read_chars(inf, buf, len))
    return 1;

  *s = buf;

  return 0;
}

static int
rs_write_string_index(FILE *savef, char * const master[],
                      int max, const char *str)
{
  int i;
  for(i = 0; i < max; i++)
    if (str == master[i])
      return rs_write_int(savef, i);

  i = -1;
  return rs_write_int(savef, i);
}

static int
rs_read_string_index(FILE *inf, char *master[], int maxindex, char **str)
{
  int i;

  if (rs_read_int(inf, &i) || i > maxindex)
    return 1;

  *str = i >= 0 ? master[i] : NULL;
  return 0;
}

static int
rs_write_window(FILE *savef, WINDOW *win)
{
  int row;
  int col;
  int height = getmaxy(win);
  int width  = getmaxx(win);

  if (rs_write_marker(savef, RSID_WINDOW) ||
      rs_write_int(savef,height) ||
      rs_write_int(savef,width))
    return 1;

  for (row = 0; row < height; row++)
    for (col = 0; col < width; col++)
    {
      int sym = mvwincch(win, row, col);
      if (rs_write_int(savef, sym) != 0)
        return 1;
    }

  return 0;
}

static int
rs_read_window(FILE *inf, WINDOW *win)
{
  size_t temp_i = 0; /* Used by rs_read_marker */
  int row;
  int col;
  int maxlines;
  int maxcols;
  int width  = getmaxx(win);
  int height = getmaxy(win);
  int value = 0;

  if (rs_read_marker(inf, RSID_WINDOW) ||
      rs_read_int(inf, &maxlines) ||
      rs_read_int(inf, &maxcols))
    return 1;

  for (row = 0; row < maxlines; row++)
    for (col = 0; col < maxcols; col++)
    {
      if (rs_read_int(inf, &value) != 0)
        return 1;
      else if ((row < height) && (col < width))
        mvwaddcch(win,row,col,value);
    }

  return 0;
}

void *
get_list_item(THING *l, int i)
{
    int count;
    for (count = 0; l != NULL; count++, l = l->l_next)
        if (count == i)
            return l;
    return NULL;
}

static int
find_list_ptr(const THING *l, const void *ptr)
{
    int count;
    for(count = 0; l != NULL; count++, l = l->l_next)
        if (l == ptr)
            return count;
    return -1;
}

static size_t
list_size(const THING *l)
{
    size_t count;
    for(count = 0; l != NULL; count++, l = l->l_next)
        ;
    return count;
}

static int
rs_write_stats(FILE *savef, const struct stats *s)
{
  size_t dmgsize = sizeof(s->s_dmg);
  if (rs_write_marker(savef, RSID_STATS) ||
      rs_write_str_t(savef, s->s_str) ||
      rs_write_int(savef, s->s_exp) ||
      rs_write_int(savef, s->s_lvl) ||
      rs_write_int(savef, s->s_arm) ||
      rs_write_int(savef, s->s_hpt) ||
      rs_write_chars(savef, s->s_dmg, dmgsize) ||
      rs_write_int(savef,s->s_maxhp))
    return 1;
  return 0;
}

static int
rs_write_stone_index(FILE *savef, const STONE master[],
                     int max, const char *str)
{
  int i;
  for(i = 0; i < max; i++)
    if (str == master[i].st_name)
      return rs_write_int(savef, i);
  i = -1;
  return rs_write_int(savef, i);
}

static int
rs_read_stone_index(FILE *inf, STONE master[], int maxindex, char **str)
{
  int i = 0;
  if (rs_read_int(inf,&i) || i > maxindex)
    return 1;

  *str = i >= 0 ? master[i].st_name : NULL;
  return 0;
}

static int
rs_write_scrolls(FILE *savef)
{
  int i;
  for (i = 0; i < NSCROLLS; i++)
    if (rs_write_string(savef, s_names[i]))
      return 1;
  return 0;
}

static int
rs_read_scrolls(FILE *inf)
{
  int i;
  for (i = 0; i < NSCROLLS; i++)
    if (rs_read_new_string(inf, &s_names[i]))
      return 1;
  return 0;
}

static int
rs_write_potions(FILE *savef)
{
  int i;
  for (i = 0; i < NPOTIONS; i++)
    if (rs_write_string_index(savef, rainbow, cNCOLORS, p_colors[i]))
      return 1;
  return 0;
}

static int
rs_read_potions(FILE *inf)
{
  int i;
  for (i = 0; i < NPOTIONS; i++)
    if (rs_read_string_index(inf, rainbow, cNCOLORS, &p_colors[i]))
      return 1;
  return 0;
}

static int
rs_write_rings(FILE *savef)
{
  int i;
  for (i = 0; i < MAXRINGS; i++)
    if (rs_write_stone_index(savef, stones, cNSTONES, r_stones[i]))
      return 1;
  return 0;
}

static int
rs_read_rings(FILE *inf)
{
  int i;
  for (i = 0; i < MAXRINGS; i++)
    if (rs_read_stone_index(inf, stones, cNSTONES, &r_stones[i]))
      return 1;
  return 0;
}

static int
rs_write_sticks(FILE *savef)
{
  int i;
  for (i = 0; i < MAXSTICKS; i++)
  {
    int staff_t = strcmp(ws_type[i], "staff");
    if (rs_write_int(savef, staff_t) ||
        rs_write_string_index(savef, staff_t ? metal : wood,
                              staff_t ? cNMETAL : cNWOOD, ws_made[i]))
      return 1;
  }
  return 0;
}

static int
rs_read_sticks(FILE *inf)
{
  int i = 0;
  for (i = 0; i < MAXSTICKS; i++)
  {
    int list = 0;
    if (rs_read_int(inf,&list) ||
        rs_read_string_index(inf, list ? metal : wood,
                             list ? cNMETAL : cNWOOD, &ws_made[i]))
      return 1;
    ws_type[i] = list ? "wand" : "staff";
  }
  return 0;
}

static int
rs_write_daemons(FILE *savef, const struct delayed_action *d_list, int count)
{
  int i = 0;

  if (rs_write_marker(savef, RSID_DAEMONS) ||
      rs_write_int(savef, count))
    return 1;

  for (i = 0; i < count; i++)
  {
    int func;
    if (d_list[i].d_func == rollwand)
      func = 1;
    else if (d_list[i].d_func == doctor)
      func = 2;
    else if (d_list[i].d_func == stomach)
      func = 3;
    else if (d_list[i].d_func == runners)
      func = 4;
    else if (d_list[i].d_func == swander)
      func = 5;
    else if (d_list[i].d_func == remove_hasted)
      func = 6;
    else if (d_list[i].d_func == remove_confusion)
      func = 7;
    else if (d_list[i].d_func == daemon_remove_true_seeing)
      func = 8;
    else if (d_list[i].d_func == cure_blindness)
      func = 9;
    else if (d_list[i].d_func == NULL)
      func = 0;
    else
      func = -1;

    if (rs_write_int(savef, d_list[i].d_type) ||
        rs_write_int(savef, func) ||
        rs_write_int(savef, d_list[i].d_arg) ||
        rs_write_int(savef, d_list[i].d_time))
      return 1;
  }
  return 0;
}

static int
rs_read_daemons(FILE *inf, struct delayed_action *d_list, int count)
{
  size_t temp_i = 0; /* Used by rs_read_marker */
  int i = 0;
  int value = 0;

  if (rs_read_marker(inf, RSID_DAEMONS) ||
      rs_read_int(inf, &value))
    return 1;

  if (value > count)
    return 1;

  for(i=0; i < count; i++)
  {
    int func = 0;
    if (rs_read_int(inf, &d_list[i].d_type) ||
        rs_read_int(inf, &func) ||
        rs_read_int(inf, &d_list[i].d_arg) ||
        rs_read_int(inf, &d_list[i].d_time))
      return 1;

    switch(func)
    {
      case 1:    d_list[i].d_func = rollwand;
      when 2:    d_list[i].d_func = doctor;
      when 3:    d_list[i].d_func = stomach;
      when 4:    d_list[i].d_func = runners;
      when 5:    d_list[i].d_func = swander;
      when 6:    d_list[i].d_func = remove_hasted;
      when 7:    d_list[i].d_func = remove_confusion;
      when 8:    d_list[i].d_func = daemon_remove_true_seeing;
      when 9:    d_list[i].d_func = cure_blindness;
      otherwise: d_list[i].d_func = NULL;
    }
  }

  if (d_list[i].d_func == NULL)
  {
    d_list[i].d_type = 0;
    d_list[i].d_arg = 0;
    d_list[i].d_time = 0;
  }
  return 0;
}

static int
rs_write_obj_info(FILE *savef, const struct obj_info *i, int count)
{
  int n;

  if (rs_write_marker(savef, RSID_MAGICITEMS) ||
      rs_write_int(savef, count))
    return 1;

  for (n = 0; n < count; n++)
    if(rs_write_int(savef,i[n].oi_prob) ||
       rs_write_int(savef,i[n].oi_worth) ||
       rs_write_string(savef,i[n].oi_guess) ||
       rs_write_boolean(savef,i[n].oi_know))
      return 1;
  return 0;
}

static int
rs_read_obj_info(FILE *inf, struct obj_info *mi, int count)
{
  size_t temp_i; /* Used by rs_read_marker */
  int n;
  int value;

  if (rs_read_marker(inf, RSID_MAGICITEMS) ||
      rs_read_int(inf, &value) ||
      value > count)
    return 1;

  for (n = 0; n < value; n++)
    if (rs_read_int(inf,&mi[n].oi_prob) ||
        rs_read_int(inf,&mi[n].oi_worth) ||
        rs_read_new_string(inf,&mi[n].oi_guess) ||
        rs_read_boolean(inf,&mi[n].oi_know))
      return 1;
  return 0;
}

static int
rs_write_rooms(FILE *savef, const struct room r[], int count)
{
  int n = 0;
  if (rs_write_int(savef, count))
    return 1;

  for (n = 0; n < count; n++)
    if (rs_write_room(savef, &r[n]))
      return 1;
  return 0;
}

static int
rs_read_rooms(FILE *inf, struct room *r, int count)
{
  int value = 0;
  int n = 0;

  if (rs_read_int(inf,&value) || value > count)
    return 1;

  for (n = 0; n < value; n++)
    if (rs_read_room(inf,&r[n]))
      return 1;
  return 0;
}

static int
rs_write_room_reference(FILE *savef, const struct room *rp)
{
  int i;
  int room = -1;

  for (i = 0; i < MAXROOMS; i++)
    if (&rooms[i] == rp)
      room = i;

  return rs_write_int(savef, room);
}

static int
rs_read_room_reference(FILE *inf, struct room **rp)
{
  int i;
  if (rs_read_int(inf, &i))
    return 1;
  *rp = &rooms[i];
  return 0;
}

static int
rs_write_object(FILE *savef, const THING *o)
{
  size_t dmgsize = sizeof(o->_o._o_damage);
  size_t hurldmgsize = sizeof(o->_o._o_hurldmg);
  if (rs_write_marker(savef, RSID_OBJECT) ||
      rs_write_int(savef, o->_o._o_type) ||
      rs_write_coord(savef, o->_o._o_pos) ||
      rs_write_int(savef, o->_o._o_launch) ||
      rs_write_char(savef, o->_o._o_packch) ||
      rs_write_chars(savef, o->_o._o_damage, dmgsize) ||
      rs_write_chars(savef, o->_o._o_hurldmg, hurldmgsize) ||
      rs_write_int(savef, o->_o._o_count) ||
      rs_write_int(savef, o->_o._o_which) ||
      rs_write_int(savef, o->_o._o_hplus) ||
      rs_write_int(savef, o->_o._o_dplus) ||
      rs_write_int(savef, o->_o._o_arm) ||
      rs_write_int(savef, o->_o._o_flags) ||
      rs_write_int(savef, o->_o._o_group) ||
      rs_write_string(savef, o->_o._o_label))
    return 1;
  return 0;
}

static int
rs_read_object(FILE *inf, THING *o)
{
  size_t temp_i = 0; /* Used by rs_read_chars && rs_read_marker */
  if (rs_read_marker(inf, RSID_OBJECT) ||
      rs_read_int(inf, &o->_o._o_type) ||
      rs_read_coord(inf, &o->_o._o_pos) ||
      rs_read_int(inf, &o->_o._o_launch) ||
      rs_read_char(inf, &o->_o._o_packch) ||
      rs_read_chars(inf, o->_o._o_damage, sizeof(o->_o._o_damage)) ||
      rs_read_chars(inf, o->_o._o_hurldmg, sizeof(o->_o._o_hurldmg)) ||
      rs_read_int(inf, &o->_o._o_count) ||
      rs_read_int(inf, &o->_o._o_which) ||
      rs_read_int(inf, &o->_o._o_hplus) ||
      rs_read_int(inf, &o->_o._o_dplus) ||
      rs_read_int(inf, &o->_o._o_arm) ||
      rs_read_int(inf, &o->_o._o_flags) ||
      rs_read_int(inf, &o->_o._o_group) ||
      rs_read_new_string(inf, &o->_o._o_label))
    return 1;
  return 0;
}

static int
rs_write_equipment(FILE *inf, THING *o, size_t marker)
{
  if (o == NULL)
    return rs_write_marker(inf, RSID_NULL);
  else if (rs_write_marker(inf, marker))
    return 1;
  return rs_write_object(inf, o);
}

static int
rs_read_equipment(FILE *inf, size_t marker)
{
  size_t temp_i;
  THING *item;

  if (rs_read_marker(inf, marker))
    return 0;
  item = new_item();
  if (rs_read_object(inf, item))
    return 1;
  equip_item(item);
  return 0;
}

static int
rs_write_object_list(FILE *savef, const THING *l)
{
  size_t listsize = list_size(l);
  if (rs_write_marker(savef, RSID_OBJECTLIST) ||
      rs_write_int(savef, listsize))
    return 1;

  for(; l != NULL; l = l->l_next)
    if (rs_write_object(savef, l))
      return 1;
  return 0;
}

static int
rs_read_object_list(FILE *inf, THING **list)
{
  size_t temp_i = 0; /* Used by rs_read_marker */
  int i;
  int cnt;
  THING *l = NULL;
  THING *previous = NULL;
  THING *head = NULL;

  if (rs_read_marker(inf, RSID_OBJECTLIST) ||
      rs_read_int(inf, &cnt))
    return 1;

  for (i = 0; i < cnt; i++)
  {
    l = new_item();
    l->l_prev = previous;

    if (previous != NULL)
      previous->l_next = l;

    if (rs_read_object(inf, l))
      return 1;

    if (previous == NULL)
      head = l;

    previous = l;
  }

  if (l != NULL)
    l->l_next = NULL;

  *list = head;

  return 0;
}

static int
rs_write_object_reference(FILE *savef, const THING *list, const THING *item)
{
  int i = find_list_ptr(list, item);
  return rs_write_int(savef, i);
}

static int
rs_read_object_reference(FILE *inf, THING *list, THING **item)
{
  int i;
  if (rs_read_int(inf, &i))
    return 1;

  *item = get_list_item(list,i);

  return 0;
}

static int
find_room_coord(struct room *rmlist, coord *c, int n)
{
  int i = 0;
  for (i = 0; i < n; i++)
    if (&rmlist[i].r_gold == c)
      return i;
  return -1;
}

static int
find_thing_coord(THING *monlist, coord *c)
{
  THING *mitem;
  int i = 0;

  for (mitem = monlist; mitem != NULL; mitem = mitem->l_next)
  {
    if (c == &mitem->t_pos)
      return i;
    i++;
  }
  return -1;
}

static int
find_object_coord(THING *objlist, coord *c)
{
  THING *oitem;
  int i = 0;

  for (oitem = objlist; oitem != NULL; oitem = oitem->l_next)
  {
    if (c == &oitem->o_pos)
      return i;
    i++;
  }
  return -1;
}

static int
rs_write_thing(FILE *savef, const THING *t)
{
  int i_zero = 0;
  int i_one = 1;

  if (rs_write_marker(savef, RSID_THING))
    return 1;

  if (t == NULL)
    return rs_write_int(savef, i_zero);

  if (rs_write_int(savef, i_one) ||
      rs_write_coord(savef, t->_t._t_pos) ||
      rs_write_boolean(savef, t->_t._t_turn) ||
      rs_write_char(savef, t->_t._t_type) ||
      rs_write_char(savef, t->_t._t_disguise) ||
      rs_write_char(savef, t->_t._t_oldch))
    return 1;


  /* 
     t_dest can be:
     0,0: NULL
     0,1: location of hero
     1,i: location of a thing (monster)
     2,i: location of an object
     3,i: location of gold in a room

     We need to remember what we are chasing rather than 
     the current location of what we are chasing.
  */


  if (t->t_dest == &hero)
  {
    if (rs_write_int(savef, i_zero) ||
        rs_write_int(savef, i_one))
      return 1;
  }
  else if (t->t_dest != NULL)
  {
    int i = find_thing_coord(mlist, t->t_dest);

    if (i >= 0)
    {
      if (rs_write_int(savef,i_one) ||
          rs_write_int(savef,i))
        return 1;
    }
    else
    {
      i = find_object_coord(lvl_obj, t->t_dest);

      if (i >= 0)
      {
        int i_two = 2;
        if (rs_write_int(savef,i_two) ||
            rs_write_int(savef,i))
          return 1;
      }
      else
      {
        i = find_room_coord(rooms, t->t_dest, MAXROOMS);

        if (i >= 0)
        {
          int i_three = 3;
          if (rs_write_int(savef,i_three) ||
              rs_write_int(savef,i))
            return 1;
        }
        else
        {
          if (rs_write_int(savef, i_zero) ||
              rs_write_int(savef,i_one)) /* chase the hero anyway */
            return 1;
        }
      }
    }
  }
  else
  {
    if (rs_write_int(savef,i_zero) ||
        rs_write_int(savef,i_zero))
      return 1;
  }

  if (rs_write_short(savef, t->_t._t_flags) ||
      rs_write_stats(savef, &t->_t._t_stats) ||
      rs_write_room_reference(savef, t->_t._t_room) ||
      rs_write_object_list(savef, t->_t._t_pack))
    return 1;
  return 0;
}

static int
rs_read_thing(FILE *inf, THING *t)
{
  size_t temp_i = 0; /* Used as buffer for macros */
  int listid = 0;
  int index = -1;

  if (rs_read_marker(inf, RSID_THING) ||
      rs_read_int(inf, &index))
    return 1;

  if (index == 0)
    return 0;

  if (rs_read_coord(inf,&t->_t._t_pos) ||
      rs_read_boolean(inf,&t->_t._t_turn) ||
      rs_read_char(inf,&t->_t._t_type) ||
      rs_read_char(inf,&t->_t._t_disguise) ||
      rs_read_char(inf,&t->_t._t_oldch))
    return 1;

  /*
     t_dest can be (listid,index):
     0,0: NULL
     0,1: location of hero
     1,i: location of a thing (monster)
     2,i: location of an object
     3,i: location of gold in a room

     We need to remember what we are chasing rather than 
     the current location of what we are chasing.
  */

  if (rs_read_int(inf, &listid) ||
      rs_read_int(inf, &index))
    return 1;
  t->_t._t_reserved = -1;

  if (listid == 0) /* hero or NULL */
  {
    if (index == 1)
      t->_t._t_dest = &hero;
    else
      t->_t._t_dest = NULL;
  }
  else if (listid == 1) /* monster/thing */
  {
    t->_t._t_dest     = NULL;
    t->_t._t_reserved = index;
  }
  else if (listid == 2) /* object */
  {
    THING *item = get_list_item(lvl_obj, index);
    if (item != NULL)
      t->_t._t_dest = &item->o_pos;
  }
  else if (listid == 3) /* gold */
  {
    t->_t._t_dest = &rooms[index].r_gold;
  }
  else
    t->_t._t_dest = NULL;

  if (rs_read_short(inf,&t->_t._t_flags) ||
      rs_read_stats(inf,&t->_t._t_stats) ||
      rs_read_room_reference(inf, &t->_t._t_room) ||
      rs_read_object_list(inf,&t->_t._t_pack))
    return 1;
  return 0;
}

static int
rs_fix_thing(THING *t)
{
  THING *item;

  if (t->t_reserved < 0)
    return 0;

  item = get_list_item(mlist,t->t_reserved);

  if (item != NULL)
    t->t_dest = &item->t_pos;
  return 0;
}

static int
rs_write_thing_list(FILE *savef, const THING *l)
{
  int cnt = list_size(l);

  if (rs_write_marker(savef, RSID_MONSTERLIST) ||
      rs_write_int(savef, cnt))
    return 1;

  if (cnt < 1)
    return 0;

  while (l != NULL) {
    if (rs_write_thing(savef, l))
      return 1;
    l = l->l_next;
  }

  return 0;
}

static int
rs_read_thing_list(FILE *inf, THING **list)
{
  size_t temp_i = 0; /* Used by rs_read_marker */
  int i;
  int cnt;
  THING *l = NULL;
  THING *previous = NULL;
  THING *head = NULL;

  if (rs_read_marker(inf, RSID_MONSTERLIST) ||
      rs_read_int(inf, &cnt))
    return 1;

  for (i = 0; i < cnt; i++)
  {
    l = new_item();
    l->l_prev = previous;

    if (previous != NULL)
      previous->l_next = l;

    if (rs_read_thing(inf,l))
      return 1;

    if (previous == NULL)
      head = l;
    previous = l;
  }

  if (l != NULL)
    l->l_next = NULL;

  *list = head;

  return 0;
}

static int
rs_fix_thing_list(THING *list)
{
  THING *item;
  for(item = list; item != NULL; item = item->l_next)
    rs_fix_thing(item);
  return 0;
}

static int
rs_write_thing_reference(FILE *savef, const THING *list, const THING *item)
{
  int i = -1;

  if (item == NULL)
  {
    if (rs_write_int(savef, i))
      return 1;
  }
  else
  {
    i = find_list_ptr(list, item);
    if (rs_write_int(savef, i))
      return 1;
  }
  return 0;
}

static int
rs_read_thing_reference(FILE *inf, THING *list, THING **item)
{
  int i;

  if (rs_read_int(inf, &i))
    return 1;

  *item = i == -1 ? NULL : get_list_item(list, i);
  return 0;
}

int
rs_save_file(FILE *savef)
{
  size_t temp_i = 0; /* Used as buffer for macros */
  size_t pack_used_size = 26;
  size_t maxstr = MAXSTR;
  size_t two_x_maxstr = 2 * MAXSTR;

  rs_assert(rs_write_boolean(savef, firstmove))
  rs_assert(rs_write_booleans(savef, pack_used, pack_used_size))
  rs_assert(rs_write_chars(savef, file_name, maxstr))
  rs_assert(rs_write_potions(savef))
  rs_assert(rs_write_chars(savef,prbuf, two_x_maxstr))
  rs_assert(rs_write_rings(savef))
  rs_assert(rs_write_char(savef, runch))
  rs_assert(rs_write_scrolls(savef))
  rs_assert(rs_write_char(savef, take))
  rs_assert(rs_write_chars(savef, whoami, maxstr))
  rs_assert(rs_write_sticks(savef))
  rs_assert(rs_write_int(savef, hungry_state))
  rs_assert(rs_write_int(savef, level))
  rs_assert(rs_write_int(savef, max_level))
  rs_assert(rs_write_int(savef, no_food))
  rs_assert(rs_write_int(savef, food_left))
  rs_assert(rs_write_int(savef, no_move))
  rs_assert(rs_write_int(savef, purse))
  rs_assert(rs_write_int(savef, quiet))
  rs_assert(rs_write_int(savef, vf_hit))
  rs_assert(rs_write_int(savef, seed))
  rs_assert(rs_write_coord(savef, stairs))
  rs_assert(rs_write_thing(savef, &player))
  rs_write_equipment(savef, equipped_item(EQUIPMENT_ARMOR), RSID_ARMOR);
  rs_write_equipment(savef, equipped_item(EQUIPMENT_RHAND), RSID_RHAND);
  rs_write_equipment(savef, equipped_item(EQUIPMENT_RRING), RSID_RRING);
  rs_write_equipment(savef, equipped_item(EQUIPMENT_LRING), RSID_LRING);
  rs_assert(rs_write_object_reference(savef, player.t_pack, l_last_pick))
  rs_assert(rs_write_object_reference(savef, player.t_pack, last_pick))
  rs_assert(rs_write_object_list(savef, lvl_obj))
  rs_assert(rs_write_thing_list(savef, mlist))
  rs_assert_write_places(savef,places,MAXLINES*MAXCOLS)
  rs_assert(rs_write_stats(savef,&max_stats))
  rs_assert(rs_write_rooms(savef, rooms, MAXROOMS))
  rs_assert(rs_write_room_reference(savef, oldrp))
  rs_assert(rs_write_rooms(savef, passages, MAXPASS))
  rs_assert(rs_write_obj_info(savef, pot_info,  NPOTIONS))
  rs_assert(rs_write_obj_info(savef, ring_info,  MAXRINGS))
  rs_assert(rs_write_obj_info(savef, scr_info,  NSCROLLS))
  rs_assert(rs_write_daemons(savef, &d_list[0], 20))
  rs_assert(rs_write_int(savef,total))
  rs_assert(rs_write_int(savef,between))
  rs_assert(rs_write_coord(savef, hero)) /* UNUSED */
  rs_assert(rs_write_int(savef, group))
  rs_assert(rs_write_window(savef,stdscr))
  return 0;
}

int
rs_restore_file(FILE *inf)
{
  size_t temp_i = 0; /* Used as buffer for macros */
  coord unused_coord;

  rs_assert(rs_read_boolean(inf, &firstmove))
  rs_assert(rs_read_booleans(inf, pack_used, 26))
  rs_assert(rs_read_chars(inf, file_name, MAXSTR))
  rs_assert(rs_read_potions(inf))
  rs_assert(rs_read_chars(inf, prbuf, 2*MAXSTR))
  rs_assert(rs_read_rings(inf))
  rs_assert(rs_read_char(inf, &runch))
  rs_assert(rs_read_scrolls(inf))
  rs_assert(rs_read_char(inf, &take))
  rs_assert(rs_read_chars(inf, whoami, MAXSTR))
  rs_assert(rs_read_sticks(inf))
  rs_assert(rs_read_int(inf, &hungry_state))
  rs_assert(rs_read_int(inf, &level))
  rs_assert(rs_read_int(inf, &max_level))
  rs_assert(rs_read_int(inf, &no_food))
  rs_assert(rs_read_int(inf, &food_left))
  rs_assert(rs_read_int(inf, &no_move))
  rs_assert(rs_read_int(inf, &purse))
  rs_assert(rs_read_int(inf, &quiet))
  rs_assert(rs_read_int(inf, &vf_hit))
  rs_assert(rs_read_int(inf, (signed *) &seed))
  rs_assert(rs_read_coord(inf, &stairs))
  rs_assert(rs_read_thing(inf, &player))
  rs_assert(rs_read_equipment(inf, RSID_ARMOR))
  rs_assert(rs_read_equipment(inf, RSID_RHAND))
  rs_assert(rs_read_equipment(inf, RSID_RRING))
  rs_assert(rs_read_equipment(inf, RSID_LRING))
  rs_assert(rs_read_object_reference(inf, player.t_pack, &l_last_pick))
  rs_assert(rs_read_object_reference(inf, player.t_pack, &last_pick))
  rs_assert(rs_read_object_list(inf, &lvl_obj))
  rs_assert(rs_read_thing_list(inf, &mlist))
  rs_assert(rs_fix_thing(&player))
  rs_assert(rs_fix_thing_list(mlist))
  rs_assert_read_places(inf,places,MAXLINES*MAXCOLS)
  rs_assert(rs_read_stats(inf, &max_stats))
  rs_assert(rs_read_rooms(inf, rooms, MAXROOMS))
  rs_assert(rs_read_room_reference(inf, &oldrp))
  rs_assert(rs_read_rooms(inf, passages, MAXPASS))
  rs_assert(rs_read_obj_info(inf, pot_info,  NPOTIONS))
  rs_assert(rs_read_obj_info(inf, ring_info,  MAXRINGS))
  rs_assert(rs_read_obj_info(inf, scr_info,  NSCROLLS))
  rs_assert(rs_read_daemons(inf, d_list, 20))
  rs_assert(rs_read_int(inf,&total))
  rs_assert(rs_read_int(inf,&between))
  rs_assert(rs_read_coord(inf, &unused_coord)) /* UNUSED */
  rs_assert(rs_read_int(inf,&group))
  rs_assert(rs_read_window(inf,stdscr))
  return 0;
}
