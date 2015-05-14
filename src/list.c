/*
 * Functions for dealing with linked lists of goodies
 *
 * @(#)list.c	4.12 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <assert.h>
#include <stdlib.h>

#include "io.h"
#include "rogue.h"

#include "list.h"

#ifndef NDEBUG
void
_assert_attached(THING *list, THING *item)
{
  assert(list != NULL);

  while (list != NULL && list != item)
    list = list->l_next;

  assert(list == item);
}
#endif

void
list_detach(THING **list, THING *item)
{
  assert(list != NULL);
  assert(*list != NULL);
  assert(item != NULL);

  if (*list == item)
    *list = item->l_next;

  if (item->l_prev != NULL)
    item->l_prev->l_next = item->l_next;

  if (item->l_next != NULL)
    item->l_next->l_prev = item->l_prev;

  item->l_next = NULL;
  item->l_prev = NULL;
}

void
_attach(THING **list, THING *item)
{
  if (*list != NULL)
  {
    item->l_next = *list;
    (*list)->l_prev = item;
    item->l_prev = NULL;
  }
  else
  {
    item->l_next = NULL;
    item->l_prev = NULL;
  }
  *list = item;
}

void
_free_list(THING **ptr)
{
  THING *item;

  while (*ptr != NULL)
  {
    item = *ptr;
    *ptr = item->l_next;
    _discard(&item);
  }
}

void
_discard(THING **item)
{
  free(*item);
  *item = NULL;
}

THING *
new_item(void)
{
  THING *item = calloc(1, sizeof *item);

  assert_or_die(item != NULL, "ran out of memory!");
  item->l_next = NULL;
  item->l_prev = NULL;
  return item;
}
