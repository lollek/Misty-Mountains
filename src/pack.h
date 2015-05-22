#ifndef _ROGUE14_PACK_H_
#define _ROGUE14_PACK_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "things.h"

int const RENAMEABLE; /* Type of item which can be renamed */

enum equipment_pos
{
  EQUIPMENT_ARMOR = 0,
  EQUIPMENT_RHAND = 1,
  EQUIPMENT_RRING = 2,
  EQUIPMENT_LRING = 3,
  NEQUIPMENT
};

/* Variables. TODO: Move these */
#define RING_SLOTS_SIZE 2
enum equipment_pos ring_slots[RING_SLOTS_SIZE];
int purse;

bool pack_save_state(void);
bool pack_load_state(void);

int8_t pack_list_index(THING const* thing);
THING* pack_list_element(int8_t i);

/* Pick up an object and add it to the pack.  If the argument is
 * non-null use it as the linked_list pointer instead of gettting
 * it off the ground. */
bool pack_add(THING* obj, bool silent);

void pack_move_msg(THING* obj); /* Message that we are standing on smt */

/* take an item out of the pack */
THING* pack_remove(THING* obj, bool newobj, bool all);

/* Add something to characters pack */
void pack_pick_up(THING* obj, bool force);

/* Find and return a magic item in the players inventory */
THING* pack_find_magic_item(void);

/* Pick something out of a pack for a purpose */
THING* pack_get_item(char const* purpose, int type);

/* Check if she's carrying anything */
bool pack_is_empty(void);

/* Counts how many items she is carrying */
unsigned pack_count_items(void);

/* Counts how many items she is carrying of a certain type */
unsigned pack_count_items_of_type(int type);

bool pack_contains_amulet(void);
bool pack_contains(THING* item);
bool pack_print_equipment(void);
bool pack_print_inventory(int type);
void pack_clear_inventory(void);

void pack_set_last_picked_item(THING* ptr);
void pack_reset_last_picked_item(void);

size_t pack_evaluate(void);

THING* pack_equipped_item(enum equipment_pos pos);

bool pack_equip_item(THING* item);
bool pack_unequip(enum equipment_pos pos, bool quiet_on_success);

bool pack_item_is_cursed(THING const* item);
void pack_curse_item(THING* item);
void pack_uncurse_item(THING* item);


#endif /* _ROGUE14_PACK_H_ */
