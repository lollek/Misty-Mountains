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
#include <stdint.h>

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
list_attach(THING **list, THING *item)
{
  assert(list != NULL);
  assert(item != NULL);

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

int8_t
list_find(THING const* list, THING const* thing)
{
  THING const* ptr;
  int8_t i;

  assert(thing != NULL);

  for (ptr = list, i = 0; ptr != NULL; ptr = ptr->l_next, ++i)
  {
    assert(i >= 0);
    if (ptr == thing)
      return i;
  }

  return -1;
}

THING *
list_element(THING *list, int8_t i)
{
  THING *ptr;

  if (i < 0)
    return NULL;

  for (ptr = list; ptr != NULL; ptr = ptr->l_next)
    if (i-- == 0)
      return ptr;

  return NULL;
}

void
list_free_all(THING **ptr)
{
  assert (ptr != NULL);

  while (*ptr != NULL)
  {
    THING *item = *ptr;
    *ptr = item->l_next;
    _discard(&item);
  }
}

void
_discard(THING **item)
{
  assert (item != NULL);

  free(*item);
  *item = NULL;
}

