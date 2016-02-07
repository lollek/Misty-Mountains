#pragma once

#include <stdlib.h>
#include <stdint.h>

#include <string>

#include "coordinate.h"

#include "things.h"

void init_pack();
void free_pack();


enum equipment_pos
{
  EQUIPMENT_ARMOR = 0,
  EQUIPMENT_RHAND = 1,
  EQUIPMENT_RRING = 2,
  EQUIPMENT_LRING = 3,
  NEQUIPMENT
};

/* Variables. TODO: Move these */
extern int const PACK_RENAMEABLE; /* Type of item which can be renamed */
#define PACK_RING_SLOTS 2

extern enum equipment_pos pack_ring_slots[PACK_RING_SLOTS];
extern int                pack_gold;

int pack_size(void);

/* Pick up an object and add it to the pack.  If the argument is
 * non-null use it as the linked_list pointer instead of gettting
 * it off the ground. */
bool pack_add(Item* obj, bool silent, bool from_floor=false);

void pack_move_msg(Item* obj); /* Message that we are standing on smt */

/* take an item out of the pack */
Item* pack_remove(Item* obj, bool newobj, bool all);

/* Add something to characters pack */
void pack_pick_up(Coordinate const&, bool force);

/* Find and return a magic item in the players inventory */
Item* pack_find_magic_item(void);

/* Pick something out of a pack for a purpose */
Item* pack_get_item(std::string const& purpose, int type);

/* Check if she's carrying anything */
bool pack_is_empty(void);

/* Counts how many items she is carrying */
int pack_count_items(void);

/* Counts how many items she is carrying of a certain type */
int pack_count_items_of_type(int type);

bool pack_contains_amulet(void);
bool pack_contains(Item* item);
bool pack_print_equipment(void);
bool pack_print_inventory(int type);
void pack_clear_inventory(void);

size_t pack_evaluate(void);

Item* pack_equipped_item(enum equipment_pos pos);

bool pack_equip_item(Item* item);
bool pack_unequip(enum equipment_pos pos, bool quiet_on_success);

Item* pack_find_arrow(void); /* Return arrow in pack or nullptr */
void pack_identify_item(void);
